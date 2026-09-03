# Device discovery validation

Validation date: 2026-09-03

## Device under test

- USB VID:PID: `2fe3:574c`
- USB product: `Ragtime Florid 00260036`
- Immutable serial: `323738373233511200260036`
- Wirelink custom name: `simulated-willow`
- Board: `dm_mc02-hil`
- FCI protocol: `0.0.1`

## Windows

Environment: Windows, Visual Studio 2026 MSVC, Ninja Multi-Config,
vcpkg `x64-windows`, libusb 1.0.30 and WinUSB selected through the firmware's
Microsoft OS 2.0 descriptors.

- Configure and release build passed with `windows-msvc-vcpkg`.
- CTest passed 4/4 tests.
- Fast physical enumeration returned the exact USB serial URI.
- Read-only identity probe reported `ready` and `compatible`.
- Default, serial and custom-name connection paths succeeded.
- `waitForDevice()` and an intentional missing-serial error succeeded.
- 20 consecutive probe cycles completed without leaving the interface busy.
- 10 consecutive connect/close cycles completed without leaving the
  interface busy.

## Linux

Environment: CachyOS Linux, libusb 1.0.30.

- The udev node was `root:uucp` mode `0660`; the test user was a member of
  `uucp`.
- The same enumeration, probe, default/serial/custom-name connection, wait and
  missing-device checks passed.
- The Python bindings enumerated and connected to the physical device and
  reported `ArmConnectionState.Ready`.
- 20 consecutive probe cycles and 10 consecutive connect/close cycles passed.
- A retained `Arm` instance observed `Reconnecting` across a complete target
  power cycle, returned to `Ready`, reacquired its expired control lease and
  successfully executed a lease-protected command.

The observed `23614 ms` recovery interval includes manual cable handling and
USB enumeration and is not a transport latency benchmark.

## MC02 USB reconnect limitation

The DM-MC-Board02 device tree enables the STM32H723 OTG HS controller with its
full-speed internal PHY on PA11/PA12, but it has no VBUS sense input. Zephyr's
STM32 UDC therefore reports that VBUS changes cannot be detected. The firmware
enables the USB device once during initialization.

When J-Link keeps the target powered, unplugging only the product USB cable may
leave the device controller in its previous session state, and reconnecting
the cable does not reliably enumerate. A complete target power cycle (product
USB and J-Link power removed) reinitializes the USB peripheral and enumerates
normally. This is a firmware/board integration limitation rather than a host
discovery or udev failure. Product hardware should expose VBUS sensing; a
firmware-only workaround would need an explicit, safely triggered UDC
disable/re-enable policy.
