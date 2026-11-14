# pyflorid

Python SDK for the Ragtime Underwater dual-arm controller.

```bash
pip install .
```

```python
from pyflorid import UnderwaterTeleop, UnderwaterArmCommand

robot = UnderwaterTeleop.connect("usb://2fe3:574c/SERIAL")
print(robot.status().info.mode)
robot.disable()
```

The module provides USB discovery, Underwater mode RPCs, zeroing, persistent
settings, firmware update, ten-motor ArmStatus telemetry, and MIT command
streaming. See the repository README and `tools/underwater_console/README.md`
for complete build and operating instructions.

## Three joints plus gripper

```python
from pyflorid import UnderwaterSingle

with UnderwaterSingle.connect("serial:///dev/ttyUSB0") as arm:
    print(arm.info())
    print(arm.read_state())
# USB uses the same API: usb://2fe3:574c/SERIAL.
```

Requires pyserial for UART and firmware 0.3.0 / Protocol 3.1. Connection is
read-only; Control is explicit. The local sender runs at 250 Hz independently
of UI refresh. Closing stops sending but does not request Damp. See
`docs/single.md` and `tools/underwater_single_console/README.md` for control,
settings, USB upgrade and mode/session ownership.
