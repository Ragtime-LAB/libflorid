from enum import Enum
from pathlib import Path
from queue import Empty
from tempfile import TemporaryDirectory
from types import SimpleNamespace
import sys
import struct
import threading
import time
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from underwater_console.controller import ConsoleController
from underwater_console.firmware import ImageVersion, image_version


def firmware_bytes(major=0, minor=9, patch=1, build=0):
    return struct.pack("<IIHHIIBBHII", 0x96F3B83D, 0, 32, 0, 3, 0,
                       major, minor, patch, build, 0) + b"abc"


class Error(Enum):
    None_ = 0
    Busy = 1


class Mode(Enum):
    Damp = 0
    Gravity = 1
    Teleop = 2
    Control = 3


class ControlKind(Enum):
    MIT = 0
    MitWithGravity = 1


class ArmCommand:
    def __init__(self):
        for field in ("q", "dq", "tau", "kp", "kd"):
            setattr(self, field, [0.0] * 10)
        self.kind = ControlKind.MIT


class Upload(Enum):
    Idle = 0
    Uploading = 1
    Completed = 2


class Result:
    def __init__(self, error=Error.None_, status=0, reason="ready"):
        self.error = error
        self.device_status = status
        self.info = SimpleNamespace(reason=reason)
        self.info.mode = Mode.Damp
        self.info.enabling_arms = 0
        self.info.upgrading = False
        for arm in ("master", "slave"):
            for suffix in ("available_mask", "enabled_mask", "disable_pending_mask", "disabled_mask"):
                setattr(self.info, arm + "_" + suffix, 0)

    def __bool__(self):
        return self.error == Error.None_ and self.device_status == 0


class FakeDevice:
    def __init__(self):
        self.calls = []
        self.uploading = False
        self.complete = False
        self.closed = False
        self.reboot_entered = threading.Event()
        self.reboot_release = threading.Event()
        self.reboot_release.set()
        self.status_samples = []
        self.disable_result = None
        self.settings_result = SimpleNamespace(error=Error.None_, device_status=0, reason="saved", snapshot=None)
        self.arm_status = None
        self.arm_writes = []
        self.entry_release = threading.Event()
        self.entry_release.set()
        self.entry_started = threading.Event()

    def close(self):
        self.closed = True
        self.calls.append("close")

    def read_arm_status(self):
        assert not self.closed
        return self.arm_status

    def write_arm_command(self, command):
        assert not self.closed
        self.arm_writes.append(command)
        return Result()

    def enter_control(self):
        self.calls.append("enter_control")
        self.entry_started.set()
        if not self.entry_release.wait(2):
            raise RuntimeError("test entry deadline")
        result = Result(Error.Busy) if self.uploading else Result()
        result.info.mode = Mode.Control
        result.info.control_session = 42
        return result

    def enter_teleop(self):
        self.calls.append("enter_teleop")
        return Result()

    def status(self):
        self.calls.append("status")
        if self.status_samples:
            return self.status_samples.pop(0)
        return Result(reason="upgrade active" if self.uploading else "ready")

    def enable(self):
        self.calls.append("enable")
        return Result(Error.Busy, 3, "upgrade active") if self.uploading else Result()

    def disable(self):
        self.calls.append("disable")
        return self.disable_result if self.disable_result is not None else Result()

    def zeroing(self):
        self.calls.append("zeroing")
        return Result(reason="Zeroing complete")

    def set_settings(self, settings):
        self.calls.append(("settings", settings))
        return self.settings_result

    def start_firmware_upload(self, image):
        self.calls.append(("upload", image))
        self.uploading = True
        return Error.None_

    def firmware_progress(self):
        if self.complete:
            self.uploading = False
        return SimpleNamespace(active=self.uploading, state=Upload.Completed if self.complete else Upload.Uploading,
                               acknowledged_bytes=3 if self.complete else 0, total_bytes=3)

    def reboot(self):
        self.calls.append("reboot")
        self.reboot_entered.set()
        if not self.reboot_release.wait(2):
            raise RuntimeError("test release deadline")
        return SimpleNamespace(request_accepted=True, departure_observed=True)


class FakeSdk:
    FirmwareUpdateError = Error
    FirmwareUploadState = Upload
    UnderwaterMode = Mode
    UnderwaterControlKind = ControlKind
    UnderwaterArmCommand = ArmCommand

    def __init__(self):
        self.device = FakeDevice()
        self.UnderwaterTeleop = SimpleNamespace(connect_device=lambda descriptor: self.device)

    def discover_devices(self, probe):
        assert probe is False
        return [SimpleNamespace(display_name="Underwater", uri="usb://2fe3:574c/TEST", serial_number="TEST")]


class ControllerTests(unittest.TestCase):
    def setUp(self):
        self.sdk = FakeSdk()
        self.controller = ConsoleController(self.sdk)
        self.directory = TemporaryDirectory()
        self.image = Path(self.directory.name) / "firmware.bin"
        self.image.write_bytes(firmware_bytes())

    def tearDown(self):
        self.sdk.device.entry_release.set()
        self.sdk.device.reboot_release.set()
        self.controller.shutdown()
        self.controller.commands.shutdown(wait=True)
        self.controller.pool.shutdown(wait=True)
        self.directory.cleanup()

    def event(self, kind):
        deadline = time.monotonic() + 2
        while time.monotonic() < deadline:
            try:
                event = self.controller.events.get(timeout=0.1)
            except Empty:
                continue
            if event.kind == kind:
                return event.value
            if event.kind == "error":
                self.fail(str(event.value))
        self.fail(f"missing {kind} event")

    def connect(self):
        self.controller.connect(SimpleNamespace(uri="TEST"))
        self.event("connected")

    def wait_for(self, predicate):
        deadline = time.monotonic() + 2
        while not predicate() and time.monotonic() < deadline:
            time.sleep(0.002)
        self.assertTrue(predicate())

    def test_control_stream_starts_after_rpc_and_publishes_ten_axis_snapshots(self):
        self.connect()
        device = self.sdk.device
        device.entry_release.clear()
        command = ArmCommand()
        command.q = [i * 0.1 for i in range(10)]
        command.kp = [20.0] * 10
        self.controller.publish_arm_command(command)
        self.controller.command("enter_control")
        self.assertTrue(device.entry_started.wait(1))
        time.sleep(0.02)
        self.assertEqual(device.arm_writes, [])
        device.entry_release.set()
        self.assertTrue(self.event("enter_control"))
        self.wait_for(lambda: len(device.arm_writes) >= 8)
        self.assertEqual(device.arm_writes[-1].q, command.q)
        other = ArmCommand()
        other.kind = ControlKind.MitWithGravity
        other.tau = [i * -0.2 for i in range(10)]
        self.controller.publish_arm_command(other)
        self.wait_for(lambda: device.arm_writes[-1] is other)
        self.assertTrue(self.controller.arm_stream_report().sending)
        self.controller.command("disable")
        self.event("disable")
        count = len(device.arm_writes)
        time.sleep(0.03)
        self.assertEqual(len(device.arm_writes), count)

    def test_busy_mode_change_keeps_control_stream_and_disconnect_joins_it(self):
        self.connect()
        device = self.sdk.device
        self.controller.command("enter_control")
        self.event("enter_control")
        self.wait_for(lambda: len(device.arm_writes) > 0)
        device.uploading = True
        count = len(device.arm_writes)
        self.controller.command("enable")
        self.assertFalse(self.event("enable"))
        self.wait_for(lambda: len(device.arm_writes) > count)
        worker = self.controller.session.arm_stream
        self.controller.disconnect()
        self.event("disconnected")
        self.assertFalse(worker.thread.is_alive())
        self.assertTrue(device.closed)
        self.assertNotIn("disable", device.calls)

    def test_old_damp_feedback_does_not_cancel_entry_but_current_exit_stops_stream(self):
        self.connect()
        device = self.sdk.device
        device.arm_status = SimpleNamespace(sequence=1, control_session=41, mode=Mode.Damp)
        self.controller.command("enter_control")
        self.event("enter_control")
        self.wait_for(lambda: len(device.arm_writes) >= 5)
        device.arm_status = SimpleNamespace(sequence=2, control_session=42, mode=Mode.Damp)
        self.wait_for(lambda: not self.controller.arm_stream_report().sending)
        count = len(device.arm_writes)
        time.sleep(0.02)
        self.assertEqual(len(device.arm_writes), count)
        self.assertNotIn("disable", device.calls)

    def test_zeroing_from_damp_rechecks_and_never_implicitly_disables(self):
        self.connect()
        self.controller.zeroing()
        result = self.event("zeroing")
        self.assertTrue(result)
        self.assertEqual(self.sdk.device.calls, ["status", "zeroing"])

    def test_zeroing_is_blocked_for_active_modes_upgrade_and_status_error(self):
        self.connect()
        for mode, upgrading, error in ((Mode.Gravity, False, Error.None_),
                                       (Mode.Teleop, False, Error.None_),
                                       (Mode.Damp, True, Error.None_),
                                       (Mode.Damp, False, Error.Busy)):
            before = Result(error)
            before.info.mode = mode
            before.info.upgrading = upgrading
            self.sdk.device.status_samples = [before]
            self.assertFalse(self.controller.zeroing_allowed(before))
            self.controller.zeroing()
            self.assertIn("先点击失能", self.event("zeroing_error"))
        self.assertNotIn("zeroing", self.sdk.device.calls)
        self.assertNotIn("disable", self.sdk.device.calls)

    def prepare_settings_feedback(self, *, ready=False):
        before, pending, confirmed = Result(), Result(), Result()
        before.info.mode = Mode.Teleop
        before.info.master_available_mask = before.info.master_enabled_mask = 3
        # Slave was offline before this action; its pending bits cannot block.
        pending.info.master_disabled_mask = 1
        pending.info.master_disable_pending_mask = 2
        pending.info.slave_disable_pending_mask = 31
        confirmed.info.master_disabled_mask = 3
        confirmed.info.slave_disable_pending_mask = 31
        self.sdk.device.status_samples = [before, confirmed]
        self.sdk.device.disable_result = confirmed if ready else pending
        return pending

    def test_settings_waits_for_fresh_disabled_axes_and_ignores_previously_offline(self):
        self.connect()
        self.prepare_settings_feedback()
        self.controller.apply_settings("new values")
        self.controller.command("enable")
        result = self.event("set_settings")
        self.assertEqual(result.reason, "saved")
        self.event("enable")
        self.assertEqual(self.sdk.device.calls,
            ["status", "disable", "status", ("settings", "new values"), "enable"])

    def test_settings_timeout_reports_missing_axes_without_write_or_retry(self):
        self.connect()
        pending = self.prepare_settings_feedback()
        self.sdk.device.status_samples[1] = pending
        self.controller.apply_settings("new values", disable_timeout_s=0.001)
        result = self.event("settings_error")
        self.assertIn("超时", result)
        self.assertIn("master缺失=0x02 slave缺失=0x00", result)
        self.assertFalse(any(isinstance(call, tuple) for call in self.sdk.device.calls))
        self.assertEqual(self.sdk.device.calls.count("disable"), 1)

    def test_settings_waits_for_enabled_motor_even_before_mapping_is_available(self):
        self.connect()
        pending = self.prepare_settings_feedback()
        self.sdk.device.status_samples[0].info.master_available_mask = 0
        self.sdk.device.status_samples[1] = pending
        self.controller.apply_settings("new values", disable_timeout_s=0.001)
        self.assertIn("master缺失=0x02", self.event("settings_error"))
        self.assertFalse(any(isinstance(call, tuple) for call in self.sdk.device.calls))

    def test_settings_disable_failure_is_reported_without_write(self):
        self.connect()
        self.sdk.device.disable_result = Result(Error.Busy, 3, "CAN feedback unavailable")
        self.controller.apply_settings("new values")
        self.assertIn("CAN feedback unavailable", self.event("settings_error"))
        self.assertEqual(self.sdk.device.calls, ["status", "disable"])

    def test_settings_requires_explicit_fresh_disabled_mask_from_firmware(self):
        self.connect()
        pending = self.prepare_settings_feedback()
        pending.info.master_disabled_mask = None
        self.controller.apply_settings("new values")
        self.assertIn("固件未提供 master", self.event("settings_error"))
        self.assertEqual(self.sdk.device.calls, ["status", "disable"])

    def test_settings_write_failure_returns_actual_business_result(self):
        self.connect()
        self.prepare_settings_feedback(ready=True)
        self.sdk.device.settings_result = SimpleNamespace(error=Error.Busy, device_status=1,
            snapshot=None, reason="upgrade active")
        self.controller.apply_settings("new values")
        result = self.event("set_settings")
        self.assertEqual((result.device_status, result.reason), (1, "upgrade active"))
        self.assertEqual(self.sdk.device.calls, ["status", "disable", ("settings", "new values")])

    def test_discovery_connect_status_and_close_do_not_change_modes(self):
        self.controller.discover()
        self.assertEqual(self.event("devices")[0].display_name, "Underwater")
        self.connect()
        self.controller.command("status")
        self.assertTrue(self.event("status"))
        self.controller.disconnect()
        self.event("disconnected")
        self.assertEqual(self.sdk.device.calls, ["status", "close"])

    def test_mode_commands_preserve_click_order(self):
        self.connect()
        for _ in range(20):
            self.controller.command("enable")
            self.controller.command("disable")
        self.controller.disconnect()
        self.event("disconnected")
        self.assertEqual(self.sdk.device.calls, ["enable", "disable"] * 20 + ["close"])

    def test_connection_requests_are_fifo_and_events_identify_their_session(self):
        seen = []
        devices = {}

        def connect(descriptor):
            seen.append(descriptor.uri)
            device = FakeDevice()
            devices[descriptor.uri] = device
            return device
        self.sdk.UnderwaterTeleop.connect_device = connect
        self.controller.connect(SimpleNamespace(uri="A"))
        self.controller.connect(SimpleNamespace(uri="B"))
        events = [self.controller.events.get(timeout=2), self.controller.events.get(timeout=2)]
        self.assertEqual(seen, ["A", "B"])
        self.assertEqual([e.value.uri for e in events], ["A", "B"])
        self.assertLess(events[0].generation, events[1].generation)
        self.assertEqual(events[1].generation, self.controller.generation)
        self.assertTrue(devices["A"].closed)
        self.assertIs(self.controller.session.device, devices["B"])

    def test_same_version_upload_reboots_without_version_gate_and_busy_is_visible(self):
        self.connect()
        self.controller.upload(self.image, reboot=True)
        self.assertEqual(self.event("upload_start"), (Error.None_, ImageVersion(0, 9, 1, 0)))
        self.controller.command("enable")
        result = self.event("enable")
        self.assertEqual(result.device_status, 3)
        self.assertEqual(result.info.reason, "upgrade active")
        self.sdk.device.complete = True
        self.controller.progress()
        reboot, version = self.event("image_reboot")
        self.assertTrue(reboot.departure_observed)
        self.assertTrue(version.matches(SimpleNamespace(major=0, minor=9, patch=1)))
        self.assertIn(("upload", firmware_bytes()), self.sdk.device.calls)
        self.assertIn("reboot", self.sdk.device.calls)
        self.assertNotIn("disable", self.sdk.device.calls)

    def test_reboot_does_not_block_control_tasks_and_close_waits_for_sdk_calls(self):
        self.connect()
        self.sdk.device.reboot_release.clear()
        self.controller.upload(self.image, reboot=True)
        self.event("upload_start")
        self.sdk.device.complete = True
        self.controller.progress()
        self.assertTrue(self.sdk.device.reboot_entered.wait(1))
        self.controller.command("status")
        self.assertTrue(self.event("status"))
        self.controller.disconnect()
        self.assertFalse(self.sdk.device.closed)
        self.sdk.device.reboot_release.set()
        self.event("disconnected")
        self.assertTrue(self.sdk.device.closed)
        self.assertNotIn("disable", self.sdk.device.calls)

    def test_uploaded_version_comes_from_upload_bytes_after_file_replacement(self):
        self.controller.inspect_image(self.image)
        self.assertEqual(self.event("image_info"), (self.image, ImageVersion(0, 9, 1, 0)))
        self.image.write_bytes(firmware_bytes(1, 2, 300, 12345))
        self.connect()
        self.controller.upload(self.image, reboot=False)
        self.assertEqual(self.event("upload_start"), (Error.None_, ImageVersion(1, 2, 300, 12345)))
        self.sdk.device.complete = True
        self.controller.progress()
        self.event("progress")
        self.assertIn(("upload", firmware_bytes(1, 2, 300, 12345)), self.sdk.device.calls)
        self.assertNotIn("reboot", self.sdk.device.calls)

    def test_unrecognized_header_does_not_add_an_upload_rejection(self):
        self.image.write_bytes(b"abc")
        self.connect()
        self.controller.upload(self.image)
        self.assertEqual(self.event("upload_start"), (Error.None_, None))
        self.assertIn(("upload", b"abc"), self.sdk.device.calls)

    def test_invalid_file_reports_error_without_upload(self):
        self.connect()
        self.controller.upload("/nonexistent-underwater-image.bin")
        self.assertIn("upload_start", self.event("error"))
        self.assertFalse(self.sdk.device.calls)


class ImageVersionTests(unittest.TestCase):
    def test_header_fields_are_little_endian_and_include_build(self):
        version = image_version(firmware_bytes(3, 12, 513, 0x12345678))
        self.assertEqual(version, ImageVersion(3, 12, 513, 0x12345678))
        self.assertEqual(str(version), "3.12.513+305419896")
        self.assertTrue(version.matches(SimpleNamespace(major=3, minor=12, patch=513)))
        self.assertFalse(version.matches(SimpleNamespace(major=3, minor=12, patch=514)))

    def test_short_or_non_mcuboot_files_have_no_version(self):
        for image in (b"", b"abc", firmware_bytes()[:28], b"\0" * 32):
            with self.subTest(image=image):
                self.assertIsNone(image_version(image))


if __name__ == "__main__":
    unittest.main()
