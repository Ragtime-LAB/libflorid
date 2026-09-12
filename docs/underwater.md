# Underwater SDK

The independent `underwater-sdk` branch/worktree adds `florid::UnderwaterTeleop`
and `pyflorid.UnderwaterTeleop` for two four-axis arms plus grippers. One transport,
one Wirelink endpoint and one owner thread carry mode RPCs and upgrades. It uses
no six-axis `Arm` control session or control lease. The default product name is
**Underwater**.

A [Python desktop console](../tools/underwater_console/README.md) provides device
selection, mode buttons, live status and local firmware file upload. Launch it
with `python3 tools/underwater_console/run.py` after building the Python SDK.

The production firmware uses USB Vendor Bulk. The host also accepts
`serial:///dev/ttyACM0` / `serial://COM3` for firmware implementing the same
Wirelink protocol on CDC. This first production firmware does not enumerate CDC;
the UART log port is not an SDK RPC port.

## Mode RPCs

```cpp
#include <florid/UnderwaterTeleop.hpp>
auto connection = florid::UnderwaterTeleop::connect("usb://2fe3:574c/SERIAL");
if (!connection) return 1; // inspect m_error, m_system_error, m_message, m_probe
auto& device = *connection.m_device;
auto enabled = device.enable();       // request Gravity
auto current = device.status();
auto teleop = device.enterTeleop();   // existing Retract -> Teleop
auto disabled = device.disable();    // request Damp
```

Use the exact URI from `discoverDevices()`, or `connect(DeviceDescriptor)`.
There is no parameterless auto-connect. A VID/PID-only URI requires one
unambiguous match; a same-session `GetUnderwaterStatus` probe must then succeed.
`create(uri)` returns null on open/probe failure; `connect` retains error details.

`UnderwaterRpcResult.m_error` reports host/transport/protocol errors or
`kDeviceRejected`. `m_device_status` retains the exact firmware business result,
or managed protocol rejection code. `m_info` contains the firmware snapshot
when it processed the request, including on a business rejection. Transport
timeouts and managed protocol rejections have no invented state snapshot.

`bool(result)` means the firmware processed and accepted the request. Motors
may still be enabling/disabling or aligning. A Teleop request can return mode
`Retract`; an enable request can show `enabling_arms` with motors still disabled.
Query `status()` to observe physical completion and the current `reason`.

| Field | Meaning |
|---|---|
| `mode` | Damp=0, Gravity=1, Teleop=2, Control=3, Zeroing=4, Retract=5 |
| `uptime_ms` | Device uptime, wrapping at 32 bits |
| `enabling_arms`, `gravity_arms` | bit 0 master, bit 1 slave |
| `master/slave_available_mask` | Fresh, discovered motor feedback |
| `master/slave_enabled_mask` | Observed enabled motors |
| `master/slave_disable_pending_mask` | Pending disable confirmations |
| `master/slave_disabled_mask` | Optional fresh feedback with raw motor status 0; absent on older firmware |
| `upgrading` | Firmware upgrade/reboot transaction active |
| `reason` | Current wait/transition/rejection detail, up to 95 UTF-8 bytes |

Motor masks use bits 0–3 for joints 1–4 and bit 4 for the gripper. Business
results: Ok=0, InvalidMode=1, InvalidState=2, Busy=3, Rejected=4, InternalError=5.
Damp, Gravity, Teleop and Control are remotely selectable. Control requires firmware v0.11.0.

Closing/destroying the SDK does not send Disable. Firmware leaves Control on
USB disconnect or command timeout; Gravity/Teleop retain their local mode.
Call `disable()` explicitly to stop. Serialize close/move with all
other calls. Blocking RPCs belong on an application thread. A timed-out mode
request is not automatically replayed: query the device to learn its outcome.
Managed RPC timeout/cancellation is local: a request already ACKed by the peer
may still execute remotely. Disconnect cancels only firmware tickets not yet
executed; it does not undo an applied Gravity/Teleop mode change. Control exits on disconnect.
A still-live timed-out operation is reaped before the next submission; while
it remains live the host reports Busy instead of leaking operation slots.

```python
import pyflorid as f
with f.UnderwaterTeleop.connect("usb://2fe3:574c/SERIAL") as device:
    result = device.enable()
    print(result.error, result.device_status)
    if result.info is not None:
        print(result.info.mode, result.info.enabling_arms, result.info.reason)
    result = device.enter_teleop()
    device.disable()
```

Python `connect_device(descriptor)` uses a discovery result. `connect` raises
`RuntimeError` for open/probe failure; `create` returns None. Mode results stay
explicit return values. Blocking SDK calls release the GIL. Context-manager
exit closes the connection only.

## Device settings

`settings()` reads the active settings and storage metadata.
`defaultSettings()` / Python `default_settings()` reads compiled defaults without
changing the device. `setSettings(settings)` / Python `set_settings(settings)`
performs one firmware transaction: write NVS, then apply the saved parameters.
Successful completion includes the actual revision and applied settings. The SDK
does not switch modes or impose a mode condition. Firmware returns Busy during
an upgrade; storage failure leaves the previous active settings in place.

```python
current = device.settings()
if current:
    settings = current.snapshot.settings
    scales = settings.angle_scale       # Python arrays are copied as lists
    scales[2] = -1.0
    settings.angle_scale = scales      # assign the edited list back
    settings.buzzer_alarm_enabled = False  # mute offline beeps, retain diagnostics
    saved = device.set_settings(settings)
    print(saved.error, saved.device_status, saved.reason)
    if saved.snapshot is not None:
        print(saved.snapshot.revision, saved.snapshot.source, saved.snapshot.storage_error)
```

The settings contain 110 floats plus `buzzer_alarm_enabled` (C++: `m_buzzer_alarm_enabled`),
a boolean that defaults to true and is persisted in NVS. Setting it false stops an
active motor-offline alarm; offline/recovery diagnostics continue. Re-enabling
resumes the alarm if motors are still offline. Firmware loads v1 records without
losing tuning, with the new switch enabled. Older SDK writes that omit this
optional wire field preserve the current preference. Fixed float array lengths:

| Fields | Array ordering / units |
|---|---|
| `angle_scale`, `angle_offset_rad`, `kp`, `kd` | 5: joint1–4, gripper; scale, rad, Nm/rad, Nm·s/rad |
| `gravity_scale` | 8: master joint1–4, then slave joint1–4; signed scale |
| `friction_coulomb_positive_nm`, `friction_coulomb_negative_nm` | 10: master joint1–4 + gripper, then slave; Nm |
| `friction_viscous_nm_per_rad_s` | Same 10 axes; Nm·s/rad |
| `friction_deadband_rad_s`, `friction_transition_rad_s` | Same 10 axes; rad/s |
| `friction_filter_time_constant_s` | Same 10 axes; seconds |
| `force_gain`, `force_low_gain` | 5: joint1–4, gripper |
| `force_low_load_nm`, `force_high_load_nm` | Scalar thresholds; Nm |
| `force_velocity_filter_s`, `force_load_filter_s` | 5: joint1–4, gripper; seconds |

`UnderwaterSettingsRpcResult` retains `error`, `device_status`, optional `snapshot`
and `reason`, including business failures. Settings business codes are Ok=0,
Busy=1, StorageFailed=2, InternalError=3; these differ from mode business codes.
Storage source is Defaults=0, Stored=1, Corrupt=2, StorageError=3. A read can return
Ok with source Corrupt/StorageError and usable compiled settings: inspect the
source, signed `storage_error` and reason. A Set timeout leaves its remote result
unknown; query settings/revision before deciding whether to retry.

The desktop settings page confirms that it will disable the arms before writing.
After confirmation, it records the currently available or enabled motor masks, sends Damp,
and waits up to 5 seconds for Damp, no enabling arms, and fresh disabled feedback
for those motors. Previously offline motors do not block. Waiting/failure reasons
include the missing masks; failure never sends Set. Canceling the confirmation
sends neither Disable nor Set. Reading defaults only fills the form. Saving is
explicit and leaves the device in Damp; no automatic re-enable is performed.
Trajectory alignment settings, zero/direction calibration and identified
self-load coefficients remain in firmware and are outside this editing API.

## Upgrade

The existing FCI upgrade client, firmware session and Wirelink bulk sender are
reused with CRC, chunk acknowledgements, cancellation and post-reboot verification.
Firmware enters Damp, waits for fresh disabled feedback from motors that were
online when the request arrived, then writes or reboots. An unmet disable
deadline rejects the request. Startup requests return device Busy with a
snapshot/reason during an upgrade; status and Damp requests still reach firmware.
The transaction ends in Damp; startup remains explicit.

`bootStatus()` queries the boot layout. `startFirmwareUpload(image)` copies
the image and returns admission status; `firmwareProgress()` and
`cancelFirmwareUpload()` support asynchronous UI integration.
`uploadFirmware(image)` blocks until staging finishes. Staging is not installation.
`reboot()` reports request acceptance and observed departure separately.
`rebootAndWait(expected_version)` checks reconnect, identity, protocol, running
version and confirmed primary slot. `updateFirmware(image, expected_version)`
uploads then verifies, retaining both results.

Existing verification requires a different major/minor/patch because FCI has
no running image hash. Same-version rebuilds can use separate upload/reboot/boot
queries but cannot claim version-based installation verification. The SDK never
reissues an uncertain upload/reboot automatically. Use a matching board's signed
image; the compatible MCUboot bootloader and writable slot must already exist.

```sh
build-underwater-sdk/examples/florid_example_underwater_teleop 'usb://2fe3:574c/SERIAL' status
build-underwater-sdk/examples/florid_example_underwater_teleop 'usb://2fe3:574c/SERIAL' upload zephyr.signed.bin
python pyflorid/examples/underwater_teleop.py 'usb://2fe3:574c/SERIAL' update --image zephyr.signed.bin --version 0.9.1
```

## Build and test

```sh
cmake -S . -B build-underwater-sdk -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON -DBUILD_PYFLORID=ON
cmake --build build-underwater-sdk -j 6
ctest --test-dir build-underwater-sdk --output-on-failure
```

Python requires pybind11 and Python development headers. For an uninstalled tree,
set `PYTHONPATH=build-underwater-sdk/pyflorid:pyflorid`. Checked-in generated
bindings include the new service; normal builds do not need WLC.
`test_underwater_teleop` exercises real fragmented COBS, managed RPC envelopes,
all business errors, actual snapshots, timeouts, close without mode/lease changes,
upgrade Busy responses and RAM bulk upload bytes/CRC. Focused executor, firmware
update, discovery, Python binding, console, and snapshot regression tests remain enabled.

## v0.11.0 ten-motor Control stream

| C++ | Python | Meaning |
|---|---|---|
| `enterControl()` | `enter_control()` | RPC-only Control entry; fresh session in response |
| `readArmStatus()` | `read_arm_status()` | Nonblocking latest `UnderwaterArmStatus` or None |
| `writeArmCommand(cmd)` | `write_arm_command(cmd)` | Queue latest `UnderwaterArmCommand`, returns error/sequence |
| `disable()` | `disable()` | Request Damp |

Order is master j1..j4, master gripper, slave j1..j4, slave gripper. All ten
q/dq/kp/kd/tau values are **motor coordinates**, with rad/rad/s/Nm units; Teleop
scale/offset do not apply. `UnderwaterControlKind.MIT` forwards the command;
`MitWithGravity` adds the existing scaled arm gravity model. Gripper gravity is
zero. No automatic friction or force reflection is added in Control.

The SDK generates command sequence, monotonic timestamp and Control session.
A successful write means local queue admission, not physical execution. State
reports `applied_command_sequence`, `applied_mask` (accepted CAN sends), and
`last_sdk_timestamp_us`, plus ten-axis q/dq/tau/temperature/status/feedback age,
validity masks, state sequence/timestamp, and command reject/overwrite counters.
The device and SDK clocks differ: the echoed SDK timestamp is usable for a
host-measured round trip; subtracting device time directly is not valid.

Publish every 4 ms. Commands are not automatically repeated by the SDK. Firmware
waits for the first command with gravity compensation and exits to Damp after
>100 ms without a new valid command, or on USB disconnect/peer replacement.
Reconnect/recovery requires `enter_control()` again; stale sessions and duplicate
sequence numbers do not renew Control. Buttons cannot enter Control; short
press leaves it for Damp and long press enters Zeroing. Upgrade ownership returns
Busy for Control entry. Configure each command before publishing it and serialize
mode changes with your producer; serialize close/move with all object access.

Status snapshots are produced at 250 Hz in every mode. Latest-value mailboxes
may skip intermediates when the host or USB is delayed; `read_arm_status()` may
also return the same sequence twice. No six-axis lease, PVT or PosVel command is
used by this interface. Wire IDs are 0x7513/0x7514; this is unreliable telemetry
over USB Bulk endpoints, separate from firmware upgrade Bulk-object transfer.

`control_stop_reason`: 0 none, 1 timeout, 2 disconnect (last Control stop).
`command_reject_reason`: 0 none, 1 mode, 2 session, 3 sequence, 4 kind, 5 numeric,
6 stale. Counts are cumulative uint32 values. Age UINT32_MAX means no feedback.

```sh
# Build this branch first; the example prefers build-underwater-sdk/pyflorid.
python examples/underwater_control.py 'usb://2fe3:574c/SERIAL'
python examples/underwater_control.py 'usb://2fe3:574c/SERIAL' --control mit-gravity --seconds 10
```

The first command only reads telemetry. The second enters Control, streams
zero-gain MitWithGravity and requests Damp when finished. The C++ equivalent
is `florid_example_underwater_control`. See the examples to fill all ten arrays.
Python array properties use STL conversion: assign a complete list, e.g.
`cmd.kp = [20.0] * 10`; modifying `cmd.kp[0]` modifies a temporary Python list.

## Set current pose as zero

`device.zeroing(timeout_ms=5000)` / `UnderwaterTeleop::zeroing(5s)` invokes the
independent Zeroing RPC (0x752a/0x752b). Firmware requires **Damp**, otherwise it
returns InvalidState and an actual-state snapshot. The SDK does not implicitly
disable or move the arms. Upgrade ownership returns Busy.

Place both arms in the desired zero pose after disabling them, then call Zeroing.
The firmware keeps motors disabled, flashes blue while calibrating, sets all ten
configured motor zeros (including grippers), and waits/retries for post-zero
feedback. `bool(reply)` means calibration completed. Offline motors count too;
retry exhaustion returns Rejected, with missing motor masks in `reply.info.reason`.
The existing first attempt plus three retries are preserved. It ends in Damp.
This operation overwrites motor zero points; it does not move to the saved zero.

The GUI “设置零点” button is enabled only after a successful Damp status (and no
upgrade). The worker rechecks the mode before sending; users must click Disable
manually first. The button is inactive during the RPC. Start the rebuilt console
with `python3 tools/underwater_console/run.py`.

A managed retry of the same call is cached, not another calibration. If a local
wait times out or USB disconnects after calibration started, the firmware still
finishes its existing sequence. Read status/logs to reconcile the outcome rather
than assuming already-written zero points were cancelled.
