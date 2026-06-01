"""Shared helpers for pyflorid example scripts."""

from __future__ import annotations

import argparse
import sys
import time
from pathlib import Path

import numpy as np


def bootstrap_import() -> None:
    repo_root = Path(__file__).resolve().parents[2]
    if str(repo_root) not in sys.path:
        sys.path.insert(0, str(repo_root))


bootstrap_import()

import pyflorid  # noqa: E402


def make_parser(description: str, *extra_args):
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument("ip", help="Robot controller IP")
    for args, kwargs in extra_args:
        parser.add_argument(*args, **kwargs)
    return parser


def apply_defaults(arm: pyflorid.Arm) -> None:
    arm.set_collision_behavior(
        np.array([20, 20, 20, 20, 10, 10], dtype=np.float32),
        np.array([20, 20, 20, 20, 10, 10], dtype=np.float32),
    )
    arm.set_watchdog_timeout(500)


def wait_for_valid_state(arm: pyflorid.Arm, timeout: float = 2.0) -> pyflorid.ArmState:
    deadline = time.time() + timeout
    state = arm.update()
    while state.time == 0.0 and time.time() < deadline:
        time.sleep(0.01)
        state = arm.update()
    return state


def ensure_safe_mode(state: pyflorid.ArmState) -> None:
    if state.mode in (pyflorid.ArmMode.Fault, pyflorid.ArmMode.EStop):
        raise RuntimeError(f"arm entered unsafe mode: {state.mode}")


def clip_torque(tau: np.ndarray, limit: np.ndarray) -> np.ndarray:
    return np.clip(tau, -limit, limit).astype(np.float32)
