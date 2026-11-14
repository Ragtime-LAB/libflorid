#!/usr/bin/env python3
"""Four-motor UART7 client. Default CLI only reads; motion requires explicit --control."""
from __future__ import annotations

import argparse
from collections import deque
from dataclasses import dataclass
import math
import struct
import time

HEADER = b"\xff\xff\xfd\x00"
SCHEMA = 3
AXES = 4
COMMAND = struct.Struct("<BBIQQ20f")
STATE = struct.Struct("<BBBBIQQIHHHHQBBBB12f4B4B4I")


def crc16(data: bytes) -> int:
    crc = 0
    for value in data:
        crc ^= value << 8
        for _ in range(8):
            crc = ((crc << 1) ^ (0x8005 if crc & 0x8000 else 0)) & 0xFFFF
    return crc


def stuff(data: bytes) -> bytes:
    result = bytearray()
    for value in data:
        result.append(value)
        if result[-3:] == b"\xff\xff\xfd":
            result.append(0xFD)
    return bytes(result)


def unstuff(data: bytes) -> bytes:
    result = bytearray()
    index = 0
    while index < len(data):
        result.append(data[index])
        index += 1
        if result[-3:] == b"\xff\xff\xfd":
            if index == len(data) or data[index] != 0xFD:
                raise ValueError("invalid byte stuffing")
            index += 1
    return bytes(result)


def encode(instruction: int, parameters: bytes = b"") -> bytes:
    body = stuff(bytes([instruction]) + parameters)
    frame = HEADER + b"\x01" + struct.pack("<H", len(body) + 2) + body
    frame += struct.pack("<H", crc16(frame))
    if len(frame) > 384:
        raise ValueError("frame exceeds 384 bytes")
    return frame


class Parser:
    def __init__(self):
        self.buffer = bytearray()
        self.candidate_since = None
        self.crc_errors = 0
        self.format_errors = 0

    def feed(self, data: bytes, now: float | None = None):
        now = time.monotonic() if now is None else now
        self.buffer.extend(data)
        packets = []
        while len(self.buffer) >= 4:
            if self.buffer[:4] != HEADER:
                del self.buffer[0]
                self.candidate_since = None
                continue
            if self.candidate_since is None:
                self.candidate_since = now
            if now - self.candidate_since > 0.020:
                del self.buffer[0]
                self.candidate_since = None
                self.format_errors += 1
                continue
            if len(self.buffer) < 7:
                break
            size = struct.unpack_from("<H", self.buffer, 5)[0] + 7
            if not 10 <= size <= 384:
                del self.buffer[0]
                self.candidate_since = None
                self.format_errors += 1
                continue
            if len(self.buffer) < size:
                break
            frame = bytes(self.buffer[:size])
            if crc16(frame[:-2]) != struct.unpack_from("<H", frame, size - 2)[0]:
                del self.buffer[0]
                self.candidate_since = None
                self.crc_errors += 1
                continue
            del self.buffer[:size]
            self.candidate_since = None
            if frame[4] != 1:
                self.format_errors += 1
                continue
            try:
                body = unstuff(frame[7:-2])
            except ValueError:
                self.format_errors += 1
                continue
            packets.append((body[0], body[1:]))
        return packets


@dataclass
class ArmStatus:
    mode: int
    kind: int
    flags: int
    sequence: int
    timestamp_us: int
    session: int
    applied_sequence: int
    applied_mask: int
    fresh_mask: int
    enabled_mask: int
    available_mask: int
    host_timestamp_us: int
    stop_reason: int
    reject_reason: int
    source: int
    enabling: int
    q: tuple[float, ...]
    dq: tuple[float, ...]
    tau: tuple[float, ...]
    temperature: tuple[int, ...]
    motor_status: tuple[int, ...]
    age_ms: tuple[int, ...]

    @classmethod
    def decode(cls, data: bytes):
        if len(data) != STATE.size or data[0] != SCHEMA:
            raise ValueError("expected four-motor protocol 3.0 state")
        values = STATE.unpack(data)
        return cls(*values[1:17], tuple(values[17:21]), tuple(values[21:25]),
                   tuple(values[25:29]), tuple(values[29:33]),
                   tuple(values[33:37]), tuple(values[37:41]))


def command_parameters(session: int, sequence: int, q, dq, kp, kd, tau,
                       kind: int = 1, host_timestamp_us: int = 0) -> bytes:
    vectors = [tuple(v) for v in (q, dq, kp, kd, tau)]
    if any(len(v) != AXES for v in vectors):
        raise ValueError("q/dq/kp/kd/tau each require four values")
    values = sum(vectors, ())
    if any(not math.isfinite(v) for v in values) or any(v < 0 for v in vectors[2] + vectors[3]):
        raise ValueError("finite MIT values and nonnegative gains required")
    if kind not in (1, 2):
        raise ValueError("kind must be MIT=1 or MitWithGravity=2")
    return COMMAND.pack(SCHEMA, kind, sequence & 0xFFFFFFFF, session, host_timestamp_us, *values)
