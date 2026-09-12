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
