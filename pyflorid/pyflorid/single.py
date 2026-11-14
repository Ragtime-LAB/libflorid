"""Four-motor single-arm API. USB Bulk and independent UART Protocol 3 share one public contract."""
from dataclasses import fields
import math
import struct
import threading
import time
from ._single_codec import ArmStatus
from ._single_serial import SerialBackend


class DeviceRejected(RuntimeError):
    def __init__(self, result, reason, snapshot):
        super().__init__(f"device result={result}: {reason}")
        self.result, self.reason, self.snapshot = result, reason, snapshot


class UnderwaterSingle:
    """Connect reads only. Control is explicit; closing never sends Damp or reconnects motion."""
    def __init__(self, uri, *, backend=None):
        self.uri = uri
        self.kind = "usb" if uri.startswith("usb://") else "serial"
        self.backend = backend
        self._lock = threading.Lock()
        self._closing = threading.Event()
        self._stream = None
        self._sequence = 0
        self._observed_session = 0
        self._producer_epoch = 0
        self.last_error = None
        self.tx_count = 0
        self._metadata = {}
        if backend is None:
            if self.kind == "usb":
                from . import _SingleUsb
                self.backend = _SingleUsb(uri)
            elif uri.startswith("serial://") and len(uri) > 9:
                self.backend = SerialBackend(uri[9:])
            else:
                raise ValueError("use usb://VID:PID/SERIAL or serial:///dev/ttyUSB0 (serial://COM3 on Windows)")
        try:
            if self.kind == "usb":
                reply = self._usb_request(0)
                self._metadata = {key: reply[key] for key in ("uid", "role", "boot_id")}
                info = self.backend.device_info()
                version = info.fw_version
                self._metadata["firmware"] = f"{version.major}.{version.minor}.{version.patch}"
            else:
                error, info = self.backend.request(1)
                self._check(error, "Ping failed", {})
                if len(info) != 25 or info[0] != 3 or info[2] != 4:
                    raise ValueError("expected single-arm Protocol 3 / four motors")
                error, identity = self.backend.request(0xa6)
                self._check(error, "GetIdentity requires dual-link firmware (Protocol 3.1)", {})
                if len(identity) != 44:
                    raise ValueError("invalid device identity response")
                self._metadata = {"uid": identity[8:40].rstrip(b"\0").decode("ascii"),
                                  "boot_id": int.from_bytes(identity[:8], "little"), "role": identity[40],
                                  "firmware": f"{info[4]}.{info[5]}.{int.from_bytes(info[6:8], 'little')}"}
        except BaseException:
            self.backend.close()
            raise
        self._sender = threading.Thread(target=self._run, name="single-command-250hz", daemon=True)
        self._sender.start()

    @classmethod
    def connect(cls, uri):
        return cls(uri)

    @staticmethod
    def _check(error, reason, snapshot):
        if error:
            raise DeviceRejected(error, reason, snapshot)

    def _usb_request(self, op, mode=0, gravity=(0, 0, 0)):
        reply = self.backend.request(op, mode, gravity)
        self._check(reply["result"], reply["reason"], reply)
        return reply

    def info(self):
        return dict(self._metadata, transport=self.kind, axis_count=4)

    def refresh_info(self):
        if self.kind == "usb":
            reply = self._usb_request(0)
            info = self.backend.device_info()
            version = info.fw_version
            self._metadata = {key: reply[key] for key in ("uid", "role", "boot_id")}
            self._metadata["firmware"] = f"{version.major}.{version.minor}.{version.patch}"
        return self.info()

    def read_state(self):
        state = self.backend.state()
        if state is None:
            return None
        return ArmStatus(**{field.name: state[field.name] for field in fields(ArmStatus)})

    def set_mode(self, mode):
        if mode not in (0, 1, 3):
            raise ValueError("modes: Damp=0, Drag=1, Control=3")
        if self.kind == "usb":
            reply = self._usb_request(1, mode)
            actual = reply["state"]
            mode_status = {"mode": actual["mode"], "session": actual["session"],
                           "source": actual["source"], "disabled_mask": reply["disabled_mask"],
                           "disable_pending_mask": reply["disable_pending_mask"]}
        else:
            error, data = self.backend.request(0xa0, bytes([mode]), priority=mode == 0)
            mode_status = self._mode_data(data)
            self._check(error, self._mode_reason(error, mode_status), mode_status)
        if mode != 3:
            self.stop_sending()
        return mode_status

    @staticmethod
    def _mode_data(data):
        if len(data) != 24:
            raise ValueError("invalid mode response")
        return {"mode": data[0], "stop_reason": data[1], "source": data[2], "enabling": data[3],
                "session": int.from_bytes(data[4:12], "little"),
                **dict(zip(("available_mask", "enabled_mask", "disable_pending_mask", "disabled_mask"),
                           struct.unpack_from("<4H", data, 12)))}

    @staticmethod
    def _mode_reason(error, data):
        if error == 5 and data.get("mode") == 3 and data.get("source") == 1:
            owner = {1: "USB", 2: "UART7"}.get(data.get("source"), "another link")
            return f"controlled by {owner}; request Damp before switching links"
        return {5: "upgrade or another request in progress", 6: "three-joint feedback/mapping not ready"}.get(error, "invalid mode request")

    def status(self):
        if self.kind == "usb":
            reply = self._usb_request(0)
            return dict(reply["state"], disabled_mask=reply["disabled_mask"], disable_pending_mask=reply["disable_pending_mask"])
        error, data = self.backend.request(0xa5)
        self._check(error, "GetMode failed", {})
        return self._mode_data(data)

    def disable(self):
        return self.set_mode(0)

    def drag(self):
        return self.set_mode(1)

    @staticmethod
    def _values(values, kind):
        values = tuple(tuple(float(x) for x in v) for v in values)
        if len(values) != 5 or any(len(v) != 4 for v in values):
            raise ValueError("q/dq/kp/kd/tau each contain four motor values")
        if kind not in (1, 2) or any(not math.isfinite(x) for v in values for x in v) or any(x < 0 for v in values[2:4] for x in v):
            raise ValueError("finite MIT values and nonnegative gains required")
        return values

    def start_control(self, q, dq, kp, kd, tau, *, kind=2):
        values = self._values((q, dq, kp, kd, tau), kind)
        with self._lock:
            epoch = self._producer_epoch
        status = self.set_mode(3)
        with self._lock:
            if epoch != self._producer_epoch or self._closing.is_set():
                return status  # A local stop/close supersedes this in-flight start. No implicit Damp.
            self._sequence = 0
            self._observed_session = 0
            self.last_error = None
            self._stream = (status["session"], kind, values)
        return status

    def update_command(self, q, dq, kp, kd, tau, *, kind=2):
        values = self._values((q, dq, kp, kd, tau), kind)
        with self._lock:
            if self._stream:
                self._stream = (self._stream[0], kind, values)

    @property
    def controlling(self):
        with self._lock:
            return self._stream is not None

    @property
    def rx_count(self):
        return self.backend.rx_count

    def _run(self):
        next_send = time.monotonic()
        while not self._closing.wait(max(0, next_send - time.monotonic())):
            now = time.monotonic()
            next_send += .004
            if next_send <= now:
                next_send = now + .004  # No catch-up burst after a scheduler pause.
            with self._lock:
                stream = self._stream
                if stream:
                    self._sequence = (self._sequence + 1) & 0xffffffff
                    sequence = self._sequence
            if not stream:
                continue
            session, kind, values = stream
            try:
                state = self.read_state()
                if state and state.session == session and state.mode == 3:
                    self._observed_session = session
                elif state and self._observed_session == session:
                    with self._lock:
                        if self._stream and self._stream[0] == session:
                            self._stream = None
                    continue
                self.backend.write_command(values, kind, session, sequence, time.monotonic_ns() // 1000)
                self.tx_count += 1
            except Exception as error:
                self.last_error = str(error)
                with self._lock:
                    self._stream = None

    def settings(self, gravity=None):
        if gravity is not None:
            gravity = tuple(float(x) for x in gravity)
            if len(gravity) != 3 or not all(math.isfinite(x) for x in gravity):
                raise ValueError("three finite gravity_scale values required")
        if self.kind == "usb":
            reply = self._usb_request(2 if gravity is None else 3, gravity=gravity or (0, 0, 0))
            return {key: reply[key] for key in ("gravity_scale", "revision", "storage_error")}
        error, data = self.backend.request(0xa7 if gravity is None else 0xa8,
                                          b"" if gravity is None else struct.pack("<3f", *gravity), timeout=5)
        if len(data) != 20:
            raise ValueError("invalid settings response")
        revision, storage_error, *values = struct.unpack("<Ii3f", data)
        result = {"gravity_scale": values, "revision": revision, "storage_error": storage_error}
        self._check(error, "settings busy or storage failed; active values returned", result)
        return result

    def start_upload(self, image):
        if self.kind != "usb":
            raise ValueError("firmware upgrade requires USB Bulk")
        return self.backend.start_upload(image)

    def firmware_progress(self):
        return self.backend.firmware_progress()

    def cancel_upload(self):
        return self.backend.cancel_upload()

    def reboot_and_wait(self, version):
        result = self.backend.reboot_and_wait(version)
        if result:
            self.refresh_info()
        return result

    def stop_sending(self):
        """Stop the local producer only; no implicit Damp or reconnect."""
        with self._lock:
            self._producer_epoch += 1
            self._stream = None

    def close(self):
        self.stop_sending()
        self._closing.set()
        self._sender.join()
        self.backend.close()

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.close()
