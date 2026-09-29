# FCI Wirelink schemas

This directory is the canonical source for the FCI Wirelink payload contract.
Generated C files belong in a CMake build directory and must not be committed.
The product contract remains FCI v1; the leading schema `version` is its
monotonic revision and is currently 7. Use WLC v0.5.0 / codegen ABI 30. Binding
profiles remain at WLC's supported profile-format version 1.

CMake generates each component's codec/bindings once, then builds its host or
firmware profile as a separate runtime target. Tests may link both roles by
assigning a distinct `RUNTIME_NAME`; production targets retain the established
`fci_<component>` generated API.

The arm component composes `arm/services.bind.wl` with the selected role file.
The shared file owns all 19 field-mapped RPCs. Host/firmware files declare
client/server capability, COBS stream framing, incoming `latest` mailboxes and
outgoing `send` routes. The latter participate in capacity calculation without
allocating receive mailboxes. CMake consumers do not need new switches.
For direct WLC use, pass both `--profile arm/services.bind.wl` and
`--profile arm/host.bind.wl` (or `firmware.bind.wl`). These deployment choices
change the profile identity and generated layout, not revision-7 payloads,
RPC delivery, message IDs or the schema identity. Products keep their existing
COBS + NONE configuration, explicit runtime storage and RPC slot counts.

## Revision 7 diagnostics

Revision 7 adds the required `ArmDiagnostics.overheat_mask`; each bit reports
the corresponding motor's over-temperature state without changing the fixed
diagnostics message ownership model.

## Revision 6 identity and transactional settings

Revision 6 adds the immutable, required `DeviceInfo.serial` identity. The
firmware derives or provisions this value; `SetDeviceInfo` can only change the
user-facing `custom_name`, so product identity and `firmware_type` cannot be
silently rewritten through the normal settings API.

On a successful `SetDeviceSettings`, the response includes the authoritative
settings that the device actually persisted and applied. Hosts must update
their cache from that response instead of assuming that the requested value
became effective. The field remains absent on error responses.

## Revision 5 bounded-metadata baseline reset

There are no deployed FCI Wirelink consumers yet, so revision 5 intentionally
uses the pre-release, pre-adoption window to bound all device names to 31 UTF-8
bytes and make diagnostic bus state/error counters exact `uint8`/`uint16`
fields. This is an incompatible contract reset: it is deliberately not checked
for compatibility with revision 4, and revisions 2 through 4 must not be used
as future migration predecessors. Revision 5 is the new baseline.

Revision 5 retains the narrow IDs and masks established by revision 4 and the
strict required-field contract established by revision 3. Operation IDs, RPC
statuses and request parameters, telemetry, settings, and every real-time
scalar/vector are `required`. Generated encoders return
`WL_CODEC_ERR_MISSING_REQUIRED_FIELD` (and measured size is `SIZE_MAX`) when a
producer omits one; decoders reject otherwise well-formed payloads missing one.
Only genuinely conditional response data remains optional: device info,
settings, motor feedback, successful register-read values, and a granted lease
token/timeout. `AcquireControlLeaseRequest.current_token` is also optional
because absence means a new acquisition rather than renewal.

- `arm/fci_arm.wl` defines the single-arm v1 wire schema.
- `arm/firmware.bind.wl` retains joint, Cartesian, and gripper control streams
  as `LATEST` and exposes the firmware-side typed RPC runtime.
- `arm/host.bind.wl` retains arm status, diagnostics, and fixed motor feedback
  as `LATEST` and exposes the host-side typed RPC runtime.
- `dual/fci_dual.wl` is the separate 12-joint/two-gripper contract with trigger
  FIFO, MIT control, typed administration, and an expiring control lease. It
  does not change the single-arm schema's fixed counts or compatibility history.
- `upgrade/fci_upgrade.wl` defines typed upgrade-control RPC plus bounded bulk
  image transfer. Its host and firmware profiles keep upgrade runtime state
  separate from arm control.

Reviewed diagnostic identities use `fnv1a64-v1`; they detect stale generated
artifacts but are not compatibility or security checks:

| Contract/profile | Identity |
| --- | --- |
| arm schema revision 7 | `0x7b0f695d2a4abfb3` |
| arm firmware profile | `0x6d71c1528ad20149` |
| arm host profile | `0xd194a0c69a5bce53` |

The schema does not define a generic acknowledgement. Wirelink reliable DATA
acknowledges link admission; each RPC response carries its own operation ID
and domain-specific application status. The legacy generic acknowledgement ID
`0x6ff0` is permanently reserved. Legacy standalone `GripperStatus` ID
`0x6003` is also reserved because that state is consolidated into other v1
messages rather than assigned a new meaning.

## Single-arm parity map

The v1 schema covers the current single-arm product semantics, but deliberately
does not provide an RPL source/API facade. Endpoint code still maps generated C
values to product-domain types, and the Wirelink wire format is intentionally
different from the legacy packed structs.

| Legacy product surface | Wirelink v1 model | Delivery/state policy |
| --- | --- | --- |
| `ArmStatus`, `ArmDiagnostics` | Same named fixed messages | Host `LATEST`, unreliable |
| `MotorFeedbackArray`, `GetMotorFeedback` | `MotorFeedback` packed arrays plus typed RPC | Host `LATEST`; reliable query/response |
| Standalone `GripperStatus` | Consolidated into `ArmStatus` and motor index 6 | No duplicate mailbox |
| Joint MIT/PosVel/Velocity/PVT | Fixed packed command messages | Firmware `LATEST`, unreliable |
| Cartesian pose/velocity | Fixed packed transform/twist commands | Firmware `LATEST`, unreliable |
| Gripper MIT/PosVel/Velocity/PVT | Fixed scalar command messages | Firmware `LATEST`, unreliable |
| Home, mode, device info/settings | Typed request/response pairs | Reliable with domain status |
| Motor register read/write/store/set-zero | Typed request/response pairs | Reliable with motor status |
| Legacy set-zero, clear-error, clear-faults | Explicit typed request/response pairs | Reliable with fault-operation status |
| Fire-and-forget emergency stop | `EmergencyStop` typed RPC | Reliable transport plus application result |
| SDK connected/disconnected packets | Acquire/release control lease | Token, bounded timeout, typed status |
| Generic `USBAck` | Removed | Link ACK plus typed application response |

Every real-time control value carries the active 64-bit lease token. A new
lease is acquired without `current_token`; presenting the current token renews
the lease. Emergency stop intentionally bypasses lease ownership so it remains
available as a safety operation. Dual-arm and upgrade protocols remain separate
schema boundaries and are not part of this parity claim.

## Remaining WLC modeling boundaries

The schema records a few constraints that must stay visible during later WLC
iterations:

- Joint IDs, register IDs, and the six/seven-bit enabled/health masks use
  `uint8`; generated decoders reject larger wire values instead of truncating
  them. Operation IDs and command/status sequences deliberately retain their
  32-bit representation.
- Device names and the immutable serial use borrowed `string<31>` views. The
  codec enforces the limit in UTF-8 bytes without copying; endpoint code only
  needs to add the terminator when mapping into 32-byte, NUL-padded storage.
- `ArmDiagnostics.bus_state` uses `uint8`, while the TX/RX error counters use
  `uint16`; uptime, tick, and mode-entry counters remain 32-bit.
- WLC has no schema imports. Shared declarations therefore live in the arm
  schema for now; the dual and upgrade directories remain separate contracts
  rather than copying arm declarations prematurely.
- Device settings are flattened into packed fixed arrays because retained/RPC
  storage cannot own an unbounded `repeated` field. This also gives the
  firmware a predictable, allocation-free representation.
