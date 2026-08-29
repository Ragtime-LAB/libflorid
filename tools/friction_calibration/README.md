# Willow friction calibration data contract

All collectors in this directory remain hardware-disabled by default. This
document describes the offline acceptance contract; it is not authorization to
enable the arm.

## Dynamic sweep logs

Schema version 3 uses firmware device time in seconds (`device_time_unit=s`).
Legacy millisecond logs are rejected rather than guessed. Every CSV binds the
data to all of the following fingerprints:

- calibrated dynamics URDF SHA-256;
- canonical collection-protocol SHA-256;
- entry/wrapper script SHA-256;
- shared base collector implementation SHA-256.

Both resume and fitting reject stale or mixed fingerprints. Sequence numbers,
firmware errors, timestamps, per-trial metadata and finite state/torque values
are audited before Pinocchio RNEA subtraction.

## Breakaway logs

Breakaway schema version 3 records the same time/unit and provenance fields,
plus the shared dynamic collector and breakaway implementation fingerprints.
Motion confirmation is based on either directed velocity or directed
displacement. The reported breakaway torque is the measured torque residual at
the first candidate motion frame, not the larger command reached after the
confirmation window. The command probe and confirmation peak remain in the
JSON for audit.

## Model selection

Repeats 0 and 1 are used for grouped training cross-validation. Repeat 2 uses a
different support posture and is final held-out evaluation only. Stribeck is
ineligible without bidirectional low-speed sweeps at two speed levels and
bidirectional training breakaway events. RNEA uses measured `q`, `dq` and an
offline smoothed `ddq`; the held-out support posture and breakaway events are
never used for model selection.
