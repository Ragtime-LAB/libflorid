# Willow position MPC

`BUILD_MPC=ON` makes `CartesianPose` commands run through the host's
`CartesianMPCSolver<WillowMPCTraits>` and sends the predicted next joint position
as `JointPVT`. `CartesianVelocities` still goes to the firmware. The model is
specific to Willow; the build flag does not select a model from device identity.

The target is a column-major 4×4 homogeneous transform. Translation is in
`m_T[12]`, `m_T[13]`, `m_T[14]`, in metres, relative to the model base. The tracked
point is the final joint origin. Rotation and `CartesianPose::m_kp/m_kd` are not
used by this position OCP; it does not hold orientation or implement Cartesian
impedance. There is no separate tool/TCP transform in this model.

The checked-in solver has five shooting intervals of 4 ms (20 ms horizon).
Call it at a nominal 250 Hz with fresh joint measurements. The active-control
example `05_cartesian_mpc` schedules that period explicitly; the callback API
itself does not enforce the MPC period.

The solver uses joint torque as its optimization input and a gravity-compensating
torque reference at the measured configuration. Only the predicted **stage 1**
joint position is commanded. The PVT speed field is a nonnegative ceiling derived
from the predicted velocity/displacement and capped at the model's joint speed
limit. The optimized torque is not sent as torque feedforward.

The configured torque limits are `[5, 5, 5, 3, 3, 3]` N·m. These existing model
limits are not a device calibration. Some Willow configurations require more
than 5 N·m just to balance gravity and cannot be held under these limits. Joint
position, velocity and torque constraints apply throughout the prediction,
including terminal state bounds. A feasible QP is not a guarantee that a target
can be held indefinitely. Motor current limits and the actual PVT servo dynamics
must also be accounted for when validating on hardware.

`CartesianMPCSolver::lastStatus()` reports zero on success, an acados status on
solver failure, or `-2` when the predicted state is nonfinite/out of bounds.
Failures produce `holdPosition(q)` and reset the failed QP's memory before reuse.
Invalid input (NaN, null pointers, malformed homogeneous transform) throws
`CommandException`. A hold command requests the measured joint position; it is
not inverse kinematics. `fallbackIK` remains as a deprecated compatibility alias.
The Arm convenience path sends the same fallback command; `lastStatus()` is
available when using the solver directly.

## Build and offline checks

```sh
git submodule update --init 3rdparty/acados
git -C 3rdparty/acados submodule update --init external/blasfeo external/hpipm
cmake -S . -B build/mpc -DBUILD_MPC=ON -DBUILD_TESTS=ON \
  -DBUILD_SHARED_LIBS=OFF -DBLASFEO_TARGET=GENERIC -DHPIPM_TARGET=GENERIC
cmake --build build/mpc --parallel
ctest --test-dir build/mpc --output-on-failure
```

The MPC tests cover solver lifetime, SDK/model consistency, stationary holding,
position tracking with an ideal joint-position servo, all prediction-stage
bounds, float/double conversion, malformed targets, infeasibility and recovery.
They do not connect to a robot or establish hardware tracking accuracy. Host CI
also runs this build under AddressSanitizer and UndefinedBehaviorSanitizer.

## Regeneration

The C solver and C++ adapter come from `scripts/templates/`. Updating their
templates, solver horizon, weights or bounds only requires Python and NumPy:

```sh
python3 scripts/urdf2mpc.py --wrappers-only --outdir generated
```

To regenerate the model, cost and gravity functions too, use the original
`Ragtime_Willow_description.urdf`, CasADi, Pinocchio with CasADi bindings, and
`acados_template` from the pinned acados submodule:

```sh
export ACADOS_SOURCE_DIR="$PWD/3rdparty/acados"
export PYTHONPATH="$ACADOS_SOURCE_DIR/interfaces/acados_template"
python3 scripts/urdf2mpc.py --urdf /path/to/Ragtime_Willow_description.urdf \
  --outdir generated
```

This invokes CasADi code generation and the local templates; it does not require
Tera. Commit the generated code with its generator changes. Changing a model or
the cost expressions requires full regeneration; `--wrappers-only` cannot change
those expressions. CasADi/Pinocchio version changes may change generated C text,
so retain a consistent generation environment when updating snapshots.
