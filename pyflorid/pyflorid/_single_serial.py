"""One owner pumps UART requests, latest commands and RX without blocking on RPCs."""
from concurrent.futures import Future
from collections import deque
from dataclasses import asdict
import struct
import threading
import time
from . import _single_codec as codec


class SerialBackend:
    kind = "serial"

    def __init__(self, port, transport=None):
        if transport is None:
            import serial
            transport = serial.Serial(port, 921600, timeout=0, write_timeout=.02,
                                      xonxoff=False, rtscts=False, dsrdtr=False)
        self.port = transport
        self.lock = threading.Lock()
        self.pending = deque()
        self.latest_command = None
        self.latest = None
        self.rx_count = self.crc_errors = self.format_errors = 0
        self.error = None
        self.request_id = 0
        self.closed = threading.Event()
        self.wake = threading.Event()
        self.thread = threading.Thread(target=self._run, name="single-uart-owner", daemon=True)
        self.thread.start()

    def request(self, op, body=b"", timeout=2, *, priority=False):
        if timeout <= 0:
            raise ValueError("timeout must be positive")
        future = Future()
        with self.lock:
            if self.closed.is_set():
                raise OSError(str(self.error or "serial connection closed"))
            self.request_id = (self.request_id + 1) & 0xffffffff
            item = (self.request_id, op, body, time.monotonic() + timeout, future, priority)
            (self.pending.appendleft if priority else self.pending.append)(item)
        self.wake.set()
        # The owner enforces the deadline, even if a prior management call is still pending.
        return future.result(timeout + .5)

    def _write(self, frame):
        if self.port.write(frame) != len(frame):
            raise OSError("short serial write; query actual mode before retrying")

    def _run(self):
        parser = codec.Parser()
        inflight = {}
        try:
            while not self.closed.is_set():
                now = time.monotonic()
                for key, active in list(inflight.items()):
                    if now >= active[3]:
                        active[4].set_exception(TimeoutError("serial response missing; execution unknown; query actual mode"))
                        del inflight[key]
                with self.lock:
                    active = None
                    if self.pending and (not inflight or (self.pending[0][5] and len(inflight) < 4)):
                        active = self.pending.popleft()
                    command, self.latest_command = self.latest_command, None
                if active:
                    if now >= active[3]:
                        active[4].set_exception(TimeoutError("management request expired before transmission"))
                    else:
                        inflight[(active[0], active[1])] = active
                        self._write(codec.encode(active[1], struct.pack("<I", active[0]) + active[2]))
                if command:
                    self._write(command)
                for instruction, data in parser.feed(self.port.read(max(1, self.port.in_waiting))):
                    if instruction == 0xa2:
                        try:
                            state = asdict(codec.ArmStatus.decode(data))
                        except ValueError:
                            parser.format_errors += 1
                            continue
                        with self.lock:
                            self.latest = state
                            self.rx_count += 1
                    elif instruction == 0x55 and len(data) >= 6:
                        key = (int.from_bytes(data[2:6], "little"), data[1])
                        active = inflight.pop(key, None)
                        if active:
                            active[4].set_result((data[0], data[6:]))
                self.crc_errors, self.format_errors = parser.crc_errors, parser.format_errors
                self.wake.wait(.0005)
                self.wake.clear()
        except Exception as error:
            self.error = error
        finally:
            self.closed.set()
            error = OSError(str(self.error or "serial connection closed"))
            for active in inflight.values():
                if not active[4].done():
                    active[4].set_exception(error)
            with self.lock:
                while self.pending:
                    item = self.pending.popleft()
                    item[4].set_exception(error)
            self.port.close()

    def state(self):
        with self.lock:
            return self.latest.copy() if self.latest is not None else None

    def write_command(self, values, kind, session, sequence, timestamp):
        frame = codec.encode(0xa1, codec.command_parameters(session, sequence, *values,
                                                          kind=kind, host_timestamp_us=timestamp))
        with self.lock:
            if self.closed.is_set():
                raise OSError(str(self.error or "serial connection closed"))
            self.latest_command = frame
        self.wake.set()

    def close(self):
        self.closed.set()
        self.wake.set()
        self.thread.join()
