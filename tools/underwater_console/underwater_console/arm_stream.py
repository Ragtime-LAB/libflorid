"""Ten-motor Bulk stream. Tk only publishes immutable command snapshots."""
from dataclasses import dataclass
import threading
import time


@dataclass(frozen=True)
class StreamReport:
    status: object = None
    sending: bool = False
    tx_hz: float = 0.0
    rx_hz: float = 0.0
    error: str = ""


def default_command(sdk):
    command = sdk.UnderwaterArmCommand()
    command.kind = sdk.UnderwaterControlKind.MitWithGravity
    command.kp = [20.0] * 10
    command.kd = [0.5] * 10
    return command


class ArmStream:
    PERIOD_S = 0.004

    def __init__(self, session, sdk):
        self.session = session
        self.sdk = sdk
        self.lock = threading.Lock()
        self.stop_event = threading.Event()
        self.command = default_command(sdk)
        self.control_session = 0
        self.report = StreamReport()
        self.thread = threading.Thread(target=self._run, name="underwater-arm-250hz", daemon=True)

    def start(self):
        self.thread.start()

    def stop(self):
        self.stop_event.set()
        self.thread.join()

    def publish(self, command):
        # The caller must not mutate a published SDK DTO.
        with self.lock:
            self.command = command

    def activate(self, control_session):
        with self.lock:
            self.control_session = control_session

    def deactivate(self):
        with self.lock:
            self.control_session = 0

    def snapshot(self):
        with self.lock:
            return self.report

    def _run(self):
        deadline = measured_at = time.monotonic()
        sent = received = 0
        tx_hz = rx_hz = 0.0
        last_sequence = None
        while not self.stop_event.is_set():
            status = None
            error = ""
            try:
                with self.session.use() as device:
                    status = device.read_arm_status()
                    with self.lock:
                        # Observe the firmware's existing Control exit. Old
                        # telemetry from before the entry RPC must not cancel it.
                        if (self.control_session and status is not None and
                                status.control_session == self.control_session and
                                status.mode != self.sdk.UnderwaterMode.Control):
                            self.control_session = 0
                        sending = bool(self.control_session)
                        if sending:
                            result = device.write_arm_command(self.command)
                            if result:
                                sent += 1
                            else:
                                error = f"发送失败：{getattr(result.error, 'name', result.error)}"
                if status is not None and status.sequence != last_sequence:
                    received += 1
                    last_sequence = status.sequence
            except Exception as exc:
                error = str(exc)
            now = time.monotonic()
            elapsed = now - measured_at
            if elapsed >= 1.0:
                tx_hz, rx_hz = sent / elapsed, received / elapsed
                sent = received = 0
                measured_at = now
            with self.lock:
                self.report = StreamReport(status, bool(self.control_session), tx_hz, rx_hz, error)
            deadline += self.PERIOD_S
            delay = deadline - time.monotonic()
            if delay < 0:
                deadline = time.monotonic()
                delay = self.PERIOD_S
            self.stop_event.wait(delay)
