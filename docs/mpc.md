# Willow position MPC

`BUILD_MPC=ON` routes `CartesianPose` targets through an internal control session:
a worker solves at **50 Hz**, and a separate worker samples a smooth joint
reference and submits `JointPVT` at **500 Hz**. Target producers do not implement
interpolation or call the solver. `CartesianVelocities` still goes to firmware.
The model is specific to Willow; firmware identity does not select the model.

The target is a column-major 4×4 homogeneous transform. Translation is in
`m_T[12..14]`, in metres relative to the model base, tracking the final joint
origin. Rotation and `CartesianPose::m_kp/m_kd` are unused. This OCP does not hold
orientation, implement Cartesian impedance, or apply a separate tool/TCP transform.

## API and example

With MPC enabled, `startCartesianPoseControl(config)` returns a
`CartesianMPCControl` with `writeOnce(target)`, `readOnce()`, `status()` and `stop()`.
The configuration is optional. `writeOnce()` publishes the latest target and
returns without waiting for a solve. Refresh the target at approximately 50 Hz,
including when holding a fixed position. No output is sent before the first target
and a valid prediction. The MPC handle's `readOnce()` returns the latest snapshot
without consuming it; `Arm::readOnce()` retains its existing fresh-sample polling
semantics (an empty state when there is no new sample).

```cpp
florid::MPCControlConfig config;
config.max_joint_velocity = 0.5f;       // rad/s
config.max_joint_acceleration = 10.0f;  // rad/s²
// Prepare a valid target from fresh feedback, as in the example.
auto control = arm->startCartesianPoseControl(config);
control->writeOnce(target); // keep refreshing from your application loop
const auto status = control->status();
// status.m_state, m_stop_reason, m_solver_status, timing/counters, q/dq reference
control->stop();
```

The `arm.control(...)` overload returning `CartesianPose` uses the same internal
session with default configuration. Its callback still runs on incoming telemetry;
multiple target updates within a planning interval are coalesced. To configure
limits or inspect status, use the active handle. Python builds with `BUILD_MPC=ON`
expose `MPCControlConfig` and `ActiveCartesianPose.status()/stop()`; timeout fields
use `datetime.timedelta`. Standard wheels continue to build with MPC disabled.
Serialize arm mode/session changes on the application's control thread. Target
writes, status reads and stopping an existing MPC session may run concurrently.

Build and run the example from the repository root:

```sh
cmake -S . -B build/mpc -DBUILD_MPC=ON -DBUILD_EXAMPLES=ON \
  -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build/mpc --parallel
# Substitute the URI of your Willow.
build/mpc/examples/florid_example_05_cartesian_mpc '<uri>' --hold-only --csv hold.csv
build/mpc/examples/florid_example_05_cartesian_mpc '<uri>' --csv motion.csv
```

The example waits up to 2 seconds for feedback, checks for device faults, a
stationary state inside model joint bounds, and gravity torque below model limits.
It uses 0.5 rad/s and 10 rad/s² limits, holds for 1 second, moves 1 cm along
model-base X and back over 4 seconds, then holds for another second. `--hold-only`
keeps the starting position for all 6 seconds. It does not home the robot or clear
faults. Start with the arm supported as required by your setup and in a feasible
Willow configuration. Motor current/torque calibration and stopping behavior need
to be validated on the device.

Console output reports solve time, command submission counts, missed periods and
stop reason. Optional CSV records targets, measured q/dq, executing q/dq references
and timing at the application's 50 Hz rate. It is not a log of every 500 Hz wire
packet. Ctrl-C, exceptions and normal completion stop the session.

## Timing, interpolation and limits

The checked-in OCP has five 20 ms shooting intervals (100 ms horizon). ERK uses
five 4 ms integration substeps per interval. The solver publishes all six q/dq
knots, anchored to the host reception time of the feedback used for that solve.
The output worker samples at elapsed prediction time, including feedback age and
solve time; it does not restart the trajectory at zero when a solve completes.
This accounts for known host-side delay, not an unsynchronized device's unknown
one-way transport delay.

Each interval uses cubic Hermite interpolation. On a new plan, a 20 ms Hermite
transition starts from the executing reference q/dq and joins the new prediction
at its corresponding future time, preserving C1 continuity. The first transition
starts at measured q/dq. MPC initialization always uses measured state. Every
spline, including transitions, is checked at its exact position/velocity extrema
and acceleration endpoints. The velocity setting also constrains the OCP; the
acceleration setting is an interpolation acceptance limit. A trajectory that
exceeds it is rejected and stops the session, rather than being clipped.
Cubic acceleration can jump at joins; this is not a jerk-limited trajectory.

`JointPVT::m_dq_limit` is a nonnegative speed ceiling, derived from reference
velocity, the 2 ms reference displacement and feedback catch-up over 20 ms, then
capped by configured/model speed limits. It is not signed velocity feedforward.
The optimizer's joint torques are not sent to the motor controller. Interpolation
and the actual PVT servo do not inherit the optimizer's torque feasibility.

| Configuration | Default | Meaning |
|---|---:|---|
| `current_limit_norm` | 0.3 | PVT normalized motor current ceiling |
| `velocity_excitation` | 1.2 | Multiplier before clamping the PVT speed ceiling |
| `max_joint_velocity` | 1.0 rad/s | OCP and interpolated reference speed limit |
| `max_joint_acceleration` | 20 rad/s² | Interpolated reference acceleration limit |
| `max_tracking_error` | 0.15 rad | Maximum measured/reference joint discrepancy |
| `state_timeout` | 40 ms | Maximum host age of joint feedback |
| `target_timeout` | 200 ms | Maximum time without a target refresh |
| `plan_timeout` | 60 ms | Maximum feedback age of a usable prediction |
| `output_lateness_limit` | 10 ms | Maximum output scheduling/work lateness |

Planning/output periods are fixed at 20/2 ms for this generated model. A custom
plan timeout must exceed 20 ms and be at most 80 ms, leaving room for the
transition inside the 100 ms horizon. An output lateness limit must be at least
2 ms and shorter than plan timeout. Targets/feedback must be finite, feedback
must stay inside position/velocity limits and report no device errors. Duplicate
sequence/timestamp samples do not refresh feedback age.

The workers exchange targets, trajectories, feedback and status through Wirelink's
preallocated `wl_latest` triple buffers. Each channel has one logical publisher
and one consumer: feedback has independent channels for the planner, output worker
and public readers; each worker publishes its own status. Newer values replace
unread older values. An unchanged trajectory is not copied again every 2 ms.
Initialization checks that the mailbox/control atomics are lock-free on the host;
no additional queue library is required.

Concurrent target writers and concurrent status/feedback readers serialize among
API callers only. Neither MPC worker takes those caller mutexes. Target updates
wake workers only for the first target; subsequent cycles use timed semaphore
waits, and the first stop/fault wakes both workers. Lifecycle/RPC changes and joins
retain cold-path locks. FCI lease access and the Wirelink executor's command outbox
still have mutexes; the full transport path is not lock-free.

`status()` combines the latest planner and output snapshots. Each snapshot is
coherent (including the output q/dq pair), but their counters need not represent
the same instant. The first stop/fault reason wins atomically. After `stop()` joins
both workers, the final counters are available. The workers skip missed periods,
without catch-up bursts or an unbounded command queue.
500 Hz is the nominal **host submission** rate, not a hard real-time guarantee or
a measured firmware execution rate. Scheduling, transport, firmware period and
servo behavior matter. A slower solve continues using the last valid trajectory
until its deadline. Longer-horizon integration can offset the CPU savings from
fewer solves; inspect actual timing on the deployment machine.

On Windows the output worker requests 1 ms timer resolution for its lifetime and
releases the request when it exits. Windows may reduce timer resolution for
occluded applications; see Microsoft's [timeBeginPeriod documentation](https://learn.microsoft.com/en-us/windows/win32/api/timeapi/nf-timeapi-timebeginperiod).
This does not elevate thread priority or replace deadline monitoring.

## Failure and lifetime behavior

A failed solve is counted and does not publish a fallback as a valid plan. The
previous valid trajectory may continue until its deadline. Stale feedback,
targets or plans, invalid curves, excessive tracking error, device errors and
transport failures latch a fault and cease output. `status()` retains the reason
and counters; subsequent target writes throw `ControlException`. Start a new
session explicitly after correcting the cause. Malformed target input throws
`CommandException` synchronously.

`stop()`, handle destruction, `MotionFinished`, `Arm::stop()`, Arm destruction,
and arm mode changes stop the MPC workers. `stop()` joins in-flight work before
returning, so no worker submits more commands after it returns. A stopped old
handle cannot restart or stop a newer session. `MotionFinished` ends output now;
it is not an instruction to wait until the last target is reached.

Ordinary joint/native Cartesian handles also capture a control generation.
Starting any new arm session (even in the same mode), stopping the arm, or a mode
operation expires old arm handles; later writes throw `ControlException`. Mode
changes wait for already-entered command submissions before issuing mode RPCs.
Ordinary joint sends check atomics instead of stopping/joining MPC on every frame.

Stopping means **ceasing host command submission**, as with the existing SDK
stop operation. It does not generate a deceleration trajectory or issue a motor
emergency-stop/disable RPC. A packet already in transport may still arrive;
physical stopping then depends on firmware command retention/watchdog behavior.
Use the device's stopping facilities for the physical test setup.

The model torque limits remain `[5, 5, 5, 3, 3, 3]` N·m, with a measured-pose
gravity compensation reference. Some configurations need more torque even just
to hold position. These model limits are not a current/torque calibration.

Direct use of `CartesianMPCSolver` remains available for offline work:
`predict()` returns the full horizon and a success flag, `lastStatus()` is 0 on
success, positive on acados failure, or -2 on invalid prediction. `solve()` is the
single-stage PVT convenience method with its measured-position hold fallback;
it does not provide the session's scheduling, watchdogs or interpolation.

## Build and offline checks

```sh
cmake -S . -B build/mpc -DBUILD_MPC=ON -DBUILD_TESTS=ON \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build/mpc --parallel
ctest --test-dir build/mpc --output-on-failure
```

The acados/HPIPM/BLASFEO C runtime is checked in under
`3rdparty/acados_runtime/` (approximately 13.64 MB). MPC needs no additional
submodules or code generation. The default `BLASFEO_TARGET=GENERIC` is portable;
an optimized backend can be chosen for the deployment CPU. See the
[runtime README](../3rdparty/acados_runtime/README.md) for supported targets,
version pins, licenses and the offline size/integrity check. The runtime libraries
are always static/PIC, including when libflorid itself is built shared.

Tests cover the actual solver, full prediction bounds, cubic interior extrema,
C1 handoffs, delayed and failed solves, target/feedback expiry, transport errors,
stopping and destruction. A Wirelink loopback peer checks mode switches, callback
completion/exceptions, duplicate feedback and concurrent public readers. Exchange
tests check coalescing, untorn snapshots, concurrent target/status callers and
in-flight submission draining when a control generation expires. A 4 ms delayed, first-order PVT
servo simulation runs the example's 1 cm out-and-back path at 50/500 Hz.
These tests do not establish hardware tracking accuracy or worst-case execution
time. Host CI also runs MPC under AddressSanitizer and UndefinedBehaviorSanitizer.

## Regeneration

The C solver and C++ adapter come from `scripts/templates/`. Updating their
templates, solver horizon, weights or bounds only requires Python and NumPy:

```sh
python3 scripts/urdf2mpc.py --wrappers-only --outdir generated
```

To regenerate the model, cost and gravity functions too, use the original
`Ragtime_Willow_description.urdf`, CasADi, Pinocchio with CasADi bindings, and
`acados_template` from a separate acados checkout matching the runtime pin:

```sh
git clone --filter=blob:none --no-checkout https://github.com/acados/acados /path/to/acados
git -C /path/to/acados checkout 4c23274e49e1304cf3c859d59ea6694ce36305a7
export ACADOS_SOURCE_DIR="/path/to/acados"
export PYTHONPATH="$ACADOS_SOURCE_DIR/interfaces/acados_template"
python3 scripts/urdf2mpc.py --urdf /path/to/Ragtime_Willow_description.urdf \
  --outdir generated
```

This invokes CasADi code generation and the local templates; it does not require
Tera or acados's nested submodules. This maintainer checkout is only needed for
full model regeneration and is not part of the normal CMake build. Existing local
`3rdparty/acados/` checkouts may be kept for this purpose; that path is now ignored.
Commit the generated code with its generator changes. Changing a model or
the cost expressions requires full regeneration; `--wrappers-only` cannot change
those expressions. CasADi/Pinocchio version changes may change generated C text,
so retain a consistent generation environment when updating snapshots.
