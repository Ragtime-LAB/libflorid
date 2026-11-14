"""Four-motor host API and asynchronous UART tests. No USB hardware is opened."""
import sys
from pathlib import Path
import struct
import threading
import time
import unittest
from types import SimpleNamespace
from dataclasses import asdict

ROOT = Path(__file__).resolve().parents[1]
if len(sys.argv) > 2 and Path(sys.argv[1]).is_dir():
    extension, package = sys.argv[1:3]
    sys.argv[1:3] = []
else:
    extension, package = ROOT / "build/pyflorid", ROOT / "pyflorid"
sys.path[:0] = [str(extension), str(package)]
import pyflorid as sdk
from pyflorid import _single_codec as codec
from pyflorid._single_serial import SerialBackend


def state_bytes(mode, session, source, sequence):
    return codec.STATE.pack(3, mode, 2, int(mode == 3), sequence, sequence*4000, session,
                            1, 15 if mode == 3 else 0, 15, 15 if mode != 0 else 0, 15,
                            0, 0, 0, source if mode == 3 else 0, 0,
                            .1, .2, .3, .4, *([0.0]*8), *([25]*4), *([1 if mode else 0]*4), *([1]*4))


class Port:
    """Device simulator responding to real encoded bytes, with deliberately slow settings writes."""
    def __init__(self):
        self.mode = 0
        self.source = 2
        self.session = 0
        self.sequence = 0
        self.gravity = [1., 1., 1.]
        self.frames = []
        self.commands = []
        self.read_buffer = bytearray()
        self.responses = []
        self.parser = codec.Parser()
        self.last_state = self.last_command = time.monotonic()
        self.lock = threading.Lock()
        self.closed = False
        self.settings_delay = .15
        self.mode_requests = []

    def mode_body(self):
        return struct.pack("<BBBBQHHHHI", self.mode, 0, self.source if self.mode == 3 else 0, 0,
                           self.session, 15, 15 if self.mode else 0, 0, 15 if not self.mode else 0, 1)

    def write(self, frame):
        with self.lock:
            self.frames.append(frame)
            for op, data in self.parser.feed(frame):
                if op == 0xa1:
                    self.commands.append(codec.COMMAND.unpack(data))
                    self.last_command = time.monotonic()
                    continue
                request_id = data[:4]
                result = 0
                body = b""
                delay = 0
                if op == 1:
                    body = bytes([3, 1, 4, 7, 0, 3, 0, 0]) + struct.pack("<QQB", 12345, self.session, 1)
                elif op == 0xa6:
                    body = struct.pack("<Q", 12345) + b"FAKE-DEVICE-UID".ljust(32, b"\0") + bytes([1, 4, 3, 1])
                elif op == 0xa0:
                    mode = data[4]
                    self.mode_requests.append(mode)
                    if self.mode == 3 and self.source == 1 and mode != 0:
                        result = 5
                    else:
                        self.mode = mode
                        self.session = self.session + 1 if mode == 3 else 0
                        self.last_command = time.monotonic()
                    body = self.mode_body()
                elif op == 0xa5:
                    body = self.mode_body()
                elif op in (0xa7, 0xa8):
                    if op == 0xa8:
                        self.gravity = list(struct.unpack_from("<3f", data, 4))
                        delay = self.settings_delay
                    body = struct.pack("<Ii3f", 1, 0, *self.gravity)
                else:
                    continue  # Exercise unknown-result timeouts.
                response = codec.encode(0x55, bytes([result, op]) + request_id + body)
                self.responses.append((time.monotonic()+delay, response))
        return len(frame)

    def flush(self):
        now = time.monotonic()
        for due, frame in self.responses[:]:
            if due <= now:
                self.read_buffer.extend(frame)
                self.responses.remove((due, frame))
        if self.mode == 3 and now-self.last_command > .1:
            self.mode, self.session = 0, 0
        if now-self.last_state >= .004:
            self.last_state = now
            self.sequence += 1
            self.read_buffer.extend(codec.encode(0xa2, state_bytes(self.mode, self.session, self.source, self.sequence)))

    @property
    def in_waiting(self):
        with self.lock:
            self.flush()
            return len(self.read_buffer)

    def read(self, n):
        with self.lock:
            self.flush()
            result = bytes(self.read_buffer[:n])
            del self.read_buffer[:n]
            return result

    def close(self):
        self.closed = True


class Usb:
    rx_count = 0
    def __init__(self):
        self.mode = 0
        self.session = 0
        self.commands = []
        self.gravity = [1., 1., 1.]
        self.rejected = False
        self.closed = False

    def state(self):
        self.rx_count += 1
        return asdict(codec.ArmStatus.decode(state_bytes(self.mode, self.session, 1, self.rx_count)))

    def request(self, operation, mode=0, gravity=(0, 0, 0)):
        if operation == 1 and not self.rejected:
            self.mode, self.session = mode, self.session+1 if mode == 3 else 0
        if operation == 3:
            self.gravity = gravity
        return {"result": 5 if self.rejected else 0, "reason": "controlled by UART7; request Damp before switching links" if self.rejected else "OK",
                "state": self.state(), "uid": "FAKE-USB-UID", "role": 2, "boot_id": 999,
                "gravity_scale": self.gravity, "revision": 1, "storage_error": 0,
                "disabled_mask": 15 if self.mode == 0 else 0, "disable_pending_mask": 0}

    def device_info(self):
        return SimpleNamespace(fw_version=SimpleNamespace(major=0, minor=3, patch=0))

    def write_command(self, *values):
        self.commands.append(values)

    def close(self):
        self.closed = True


class SingleTests(unittest.TestCase):
    def serial_device(self):
        port = Port()
        backend = SerialBackend("unused", transport=port)
        return sdk.UnderwaterSingle("serial://fake", backend=backend), port

    def test_stop_supersedes_inflight_control_without_restarting_producer(self):
        for stop in ("stop_sending", "disable"):
            with self.subTest(stop=stop):
                backend = Usb()
                device = sdk.UnderwaterSingle("usb://fake", backend=backend)
                entered, release = threading.Event(), threading.Event()
                original = backend.request
                def delayed(operation, *args, **kwargs):
                    if operation == 1 and args and args[0] == 3:
                        # The device has entered Control; delay only host completion.
                        result = original(operation, *args, **kwargs)
                        entered.set()
                        release.wait(1)
                        return result
                    return original(operation, *args, **kwargs)
                backend.request = delayed
                worker = threading.Thread(target=lambda: device.start_control(*([[0]*4]*5)))
                try:
                    worker.start()
                    self.assertTrue(entered.wait(1))
                    getattr(device, stop)()
                    release.set()
                    worker.join(1)
                    self.assertFalse(worker.is_alive())
                    time.sleep(.02)
                    self.assertFalse(device.controlling)
                    self.assertEqual(backend.commands, [])
                    # Local stop leaves firmware's timeout; explicit disable requests Damp.
                    self.assertEqual(backend.mode, 3 if stop == "stop_sending" else 0)
                finally:
                    release.set()
                    worker.join()
                    device.close()

    def test_serial_identity_read_only_and_four_motor_telemetry(self):
        device, port = self.serial_device()
        try:
            self.assertEqual(device.info()["uid"], "FAKE-DEVICE-UID")
            self.assertEqual(device.info()["axis_count"], 4)
            time.sleep(.03)
            state = device.read_state()
            self.assertEqual(len(state.q), 4)
            self.assertAlmostEqual(state.q[3], .4)
            self.assertEqual(port.mode_requests, [])
            self.assertEqual(port.commands, [])
        finally:
            device.close()

    def test_slow_serial_settings_do_not_interrupt_control(self):
        device, port = self.serial_device()
        try:
            device.start_control(*([[0]*4]*5))
            values = device.settings((.5, -.25, 1.2))
            self.assertGreater(len(port.commands), 20)
            self.assertTrue(device.controlling)
            self.assertEqual(port.mode, 3)
            self.assertAlmostEqual(values["gravity_scale"][1], -.25)
            self.assertTrue(all(len(command) == 25 for command in port.commands))  # 5 header fields + 20 floats
            device.disable()
            self.assertFalse(device.controlling)
            self.assertEqual(port.mode, 0)
        finally:
            device.close()

    def test_damp_is_not_queued_behind_a_slow_serial_settings_reply(self):
        device, port = self.serial_device()
        try:
            port.settings_delay = .25
            device.start_control(*([[0]*4]*5))
            completed = []
            worker = threading.Thread(target=lambda: completed.append(device.settings((.5, .6, .7))))
            worker.start()
            deadline = time.monotonic() + .2
            while not any(op == 0xa8 for op, _ in codec.Parser().feed(b"".join(port.frames))):
                if time.monotonic() > deadline:
                    self.fail("settings request was not sent")
                time.sleep(.001)
            started = time.monotonic()
            device.disable()
            self.assertLess(time.monotonic()-started, .1)
            self.assertEqual(port.mode, 0)
            self.assertFalse(device.controlling)
            worker.join()
            self.assertEqual(len(completed), 1)
        finally:
            device.close()

    def test_serial_mode_rejection_retains_owner_and_is_actionable(self):
        device, port = self.serial_device()
        try:
            port.mode, port.session, port.source = 3, 42, 1
            with self.assertRaises(sdk.DeviceRejected) as ctx:
                device.start_control(*([[0]*4]*5))
            self.assertIn("Damp", str(ctx.exception))
            self.assertEqual(ctx.exception.snapshot["source"], 1)
            self.assertFalse(device.controlling)
            device.disable()
            self.assertEqual(port.mode, 0)
        finally:
            device.close()

    def test_serial_timeout_is_not_automatically_replayed(self):
        device, port = self.serial_device()
        try:
            with self.assertRaises(TimeoutError):
                device.backend.request(0xff, timeout=.03)
            self.assertEqual(sum(op == 0xff for op, _ in codec.Parser().feed(b"".join(port.frames))), 1)
            self.assertEqual(len(device.settings()["gravity_scale"]), 3)
        finally:
            device.close()

    def test_usb_same_api_business_failure_and_no_implicit_resume(self):
        backend = Usb()
        device = sdk.UnderwaterSingle("usb://fake", backend=backend)
        try:
            self.assertEqual(device.info()["role"], 2)
            self.assertEqual(backend.commands, [])
            device.start_control(*([[0]*4]*5))
            time.sleep(.03)
            self.assertGreater(len(backend.commands), 3)
            self.assertEqual(len(backend.commands[0][0][0]), 4)
            backend.mode, backend.session = 0, 0
            time.sleep(.02)
            self.assertFalse(device.controlling)
            backend.rejected = True
            with self.assertRaises(sdk.DeviceRejected):
                device.start_control(*([[0]*4]*5))
            self.assertFalse(device.controlling)
        finally:
            device.close()
        self.assertTrue(backend.closed)

    def test_invalid_four_motor_commands_and_serial_upgrade(self):
        device, port = self.serial_device()
        try:
            with self.assertRaises(ValueError):
                device.start_control(*([[0]*5]*5))
            with self.assertRaises(ValueError):
                device.settings((1, 2, float("nan")))
            with self.assertRaises(ValueError):
                device.start_upload(b"image")
            self.assertEqual(port.mode_requests, [])
        finally:
            device.close()


if __name__ == "__main__":
    unittest.main()
