"""Background SDK calls. No Tk objects are touched outside the UI thread."""
from concurrent.futures import ThreadPoolExecutor
from contextlib import contextmanager
from dataclasses import dataclass
from pathlib import Path
from queue import Queue
import threading
import time

from .firmware import image_version
from .arm_stream import ArmStream


@dataclass(frozen=True)
class Event:
    kind: str
    generation: int | None
    value: object


class Session:
    """Keep the SDK object alive until in-flight calls finish before close."""

    def __init__(self, device, generation):
        self.device = device
        self.generation = generation
        self.condition = threading.Condition()
        self.active = 0
        self.closing = False
        self.arm_stream = None

    @contextmanager
    def use(self):
        with self.condition:
            if self.closing:
                raise RuntimeError("连接正在关闭")
            self.active += 1
        try:
            yield self.device
        finally:
            with self.condition:
                self.active -= 1
                self.condition.notify_all()

    def close(self):
        if self.arm_stream:
            self.arm_stream.stop()
        with self.condition:
            self.closing = True
            self.condition.wait_for(lambda: self.active == 0)
        self.device.close()


class ConsoleController:
    def __init__(self, sdk=None):
        if sdk is None:
            import pyflorid as sdk
        self.sdk = sdk
        self.events = Queue()
        self.pool = ThreadPoolExecutor(max_workers=4, thread_name_prefix="underwater-usb")
        # Preserve click order for mode requests and connection lifecycle.
        self.commands = ThreadPoolExecutor(max_workers=1, thread_name_prefix="underwater-commands")
        self.lock = threading.Lock()
        self.lifecycle_lock = threading.Lock()
        self.session = None
        self.generation = 0
        self.pending = set()
        self.uploads = {}
        self.closed = False

    @property
    def connected(self):
        with self.lock:
            return self.session is not None

    def _submit(self, kind, action, generation=None, coalesce=False, parallel=False):
        key = (kind, generation)
        with self.lock:
            if self.closed or (coalesce and key in self.pending):
                return
            self.pending.add(key)

        def run():
            try:
                value = action()
                self.events.put(value if isinstance(value, Event) else Event(kind, generation, value))
            except Exception as exc:
                self.events.put(Event("error", generation, f"{kind}: {exc}"))
            finally:
                with self.lock:
                    self.pending.discard(key)
        (self.pool if parallel else self.commands).submit(run)

    def discover(self):
        # Enumeration only; never open another live device merely to name it.
        self._submit("devices", lambda: self.sdk.discover_devices(probe=False), coalesce=True, parallel=True)

    def connect(self, descriptor):
        def action():
            with self.lifecycle_lock:
                self._close_session()
                device = self.sdk.UnderwaterTeleop.connect_device(descriptor)
                with self.lock:
                    self.session = Session(device, self.generation)
                    self.session.arm_stream = ArmStream(self.session, self.sdk)
                    self.session.arm_stream.start()
                return Event("connected", self.generation, descriptor)
        self._submit("connected", action)

    def _close_session(self):
        with self.lock:
            previous = self.session
            self.session = None
            self.generation += 1
            if previous:
                self.uploads.pop(previous.generation, None)
        if previous:
            previous.close()  # SDK close does not send a motor mode command.

    def disconnect(self):
        def action():
            with self.lifecycle_lock:
                self._close_session()
                return Event("disconnected", self.generation, None)
        self._submit("disconnected", action)

    def _capture(self):
        with self.lock:
            session = self.session
        if session is None:
            self.events.put(Event("error", None, "未连接设备"))
        return session

    def command(self, method, coalesce=False):
        session = self._capture()
        if session is None:
            return

        def action():
            with session.use() as device:
                result = getattr(device, method)()
                if result and session.arm_stream:
                    if method == "enter_control":
                        session.arm_stream.activate(result.info.control_session)
                    elif method in ("enable", "disable", "enter_teleop"):
                        session.arm_stream.deactivate()
                return result
        self._submit(method, action, session.generation, coalesce,
                     parallel=method in ("boot_status", "reboot", "cancel_firmware_upload"))

    def publish_arm_command(self, command):
        with self.lock:
            if self.session and self.session.arm_stream:
                self.session.arm_stream.publish(command)

    def arm_stream_report(self):
        with self.lock:
            if self.session and self.session.arm_stream:
                return self.session.arm_stream.snapshot()
        return None

    def poll(self):
        if self.connected:
            self.command("status", coalesce=True)
            self.progress()

    def zeroing_allowed(self, result):
        info = getattr(result, "info", None)
        return bool(result) and info is not None and info.mode == self.sdk.UnderwaterMode.Damp and not info.upgrading

    def zeroing(self):
        session = self._capture()
        if session is None:
            return

        def action():
            try:
                with session.use() as device:
                    before = device.status()
                    self.events.put(Event("status", session.generation, before))
                    if not self.zeroing_allowed(before):
                        return Event("zeroing_error", session.generation,
                            "设置零点未执行：请先点击失能，等待设备处于 Damp，升级期间不可校准。")
                    self.events.put(Event("message", session.generation,
                        "正在将两臂全部电机（含夹爪）的当前位置设为零点，等待电机反馈…"))
                    return device.zeroing()
            finally:
                self.events.put(Event("zeroing_done", session.generation, None))
        self._submit("zeroing", action, session.generation, coalesce=True)

    def apply_settings(self, settings, disable_timeout_s=5.0):
        """Called only after the UI's explicit disable confirmation.

        Capture the current session and serialize the complete read/disable/wait/
        write transaction with mode commands. A failed wait never sends Set.
        """
        session = self._capture()
        if session is None:
            return

        def failure(stage, result):
            info = getattr(result, "info", None)
            return Event("settings_error", session.generation,
                f"{stage}失败：{result.error} status={result.device_status} "
                f"{getattr(info, 'reason', '')}；未提交设置。")

        def action():
            with session.use() as device:
                before = device.status()
                if not before or before.info is None:
                    return failure("读取失能前状态", before)
                required = {arm: (getattr(before.info, arm + "_available_mask") |
                                  getattr(before.info, arm + "_enabled_mask"))
                            for arm in ("master", "slave")}
                result = device.disable()
                if result and session.arm_stream:
                    session.arm_stream.deactivate()
                self.events.put(Event("settings_mode", session.generation, result))
                if not result or result.info is None:
                    return failure("失能请求", result)
                deadline = time.monotonic() + disable_timeout_s
                next_report = 0.0
                while True:
                    with self.lock:
                        if self.closed:
                            return Event("settings_error", session.generation, "窗口已关闭；未提交设置。")
                    info = result.info
                    missing = {}
                    for arm in required:
                        disabled = getattr(info, arm + "_disabled_mask", None)
                        if disabled is None and required[arm]:
                            return Event("settings_error", session.generation,
                                f"固件未提供 {arm} 的新鲜失能反馈位图，无法确认本次在线电机失能；未提交设置。")
                        missing[arm] = required[arm] & (~(disabled or 0) |
                            getattr(info, arm + "_enabled_mask") |
                            getattr(info, arm + "_disable_pending_mask"))
                    damp = info.mode == self.sdk.UnderwaterMode.Damp and info.enabling_arms == 0
                    if damp and not any(missing.values()):
                        self.events.put(Event("settings_wait", session.generation,
                            "本次在线电机已确认失能，正在保存并应用设置…"))
                        return device.set_settings(settings)
                    detail = (f"等待 Damp / 电机失能：mode={info.mode} enabling=0x{info.enabling_arms:X} "
                              f"master缺失=0x{missing['master']:02X} slave缺失=0x{missing['slave']:02X}" +
                              (f" · {info.reason}" if info.reason else ""))
                    now = time.monotonic()
                    if now >= deadline:
                        return Event("settings_error", session.generation,
                            f"失能等待超时（{disable_timeout_s:g}s）；{detail}；未提交设置。")
                    if now >= next_report:
                        self.events.put(Event("settings_wait", session.generation, detail))
                        next_report = now + 1.0
                    time.sleep(min(0.1, deadline - now))
                    result = device.status()
                    self.events.put(Event("settings_mode", session.generation, result))
                    if not result or result.info is None:
                        return failure("等待失能状态", result)
        self._submit("set_settings", action, session.generation)

    def inspect_image(self, path):
        def action():
            try:
                with Path(path).open("rb") as source:
                    return path, image_version(source.read(32))
            except OSError as exc:
                return Event("image_info_error", None, (path, str(exc)))
        self._submit("image_info", action, parallel=True)

    def upload(self, path, reboot=True):
        session = self._capture()
        if session is None:
            return

        def action():
            image = Path(path).read_bytes()
            # Parse the bytes actually uploaded, even if the file changed after selection.
            version = image_version(image)
            with self.lock:
                with session.use() as device:
                    result = device.start_firmware_upload(image)
                if result == self.sdk.FirmwareUpdateError.None_:
                    self.uploads[session.generation] = (reboot, version)
            return result, version
        self._submit("upload_start", action, session.generation, parallel=True)

    def progress(self):
        session = self._capture()
        if session is None:
            return

        def action():
            policy = None
            with self.lock:
                with session.use() as device:
                    result = device.firmware_progress()
                if not result.active:
                    policy = self.uploads.pop(session.generation, None)
            if policy and result.state == self.sdk.FirmwareUploadState.Completed and policy[0]:
                _, version = policy

                def reboot_action():
                    with session.use() as device:
                        return device.reboot(), version
                self.events.put(Event("message", session.generation,
                    "固件写入完成，正在重启…"))
                self._submit("image_reboot", reboot_action, session.generation, parallel=True)
            return result
        self._submit("progress", action, session.generation, coalesce=True, parallel=True)

    def cancel_upload(self):
        session = self._capture()
        if session is None:
            return
        with self.lock:
            self.uploads.pop(session.generation, None)
        self.command("cancel_firmware_upload")

    def shutdown(self):
        # A lifecycle job waits for in-flight SDK calls; no concurrent C++ close.
        with self.lock:
            if self.closed:
                return
            self.closed = True

        def finish():
            with self.lifecycle_lock:
                self._close_session()
        self.commands.submit(finish)
        self.commands.shutdown(wait=False)
        self.pool.shutdown(wait=False)
