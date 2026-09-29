# Dual-arm contract

`fci_dual.wl` directly replaces the app-local RPL copy used by
`piperx_dual`. It stays separate from the six-joint arm contract so neither
product pays for the other's fixed arrays or compatibility history.

The firmware profile retains the newest 12-joint and two-gripper command
independently. The host profile uses LATEST for arm telemetry and a bounded
FIFO for trigger events; trigger sequence numbers expose loss without making
the real-time producer wait for a link acknowledgement. Session-connected
packets are replaced by an expiring control lease, and every administrative
operation has a typed RPC result rather than a generic USB ACK.

Joint indices `0..5` are arm 1 and `6..11` arm 2; gripper indices `0` and `1`
follow the same order. The v1 contract intentionally exposes only the product's
implemented PC/Drag/Damp arm modes and MIT motor control. Both MIT messages keep
the frozen product's `uint16` sequence, make dt/gravity explicit, and carry the
active 64-bit lease token. Future modes can be added compatibly when the product
implements them.

The nine reliable typed RPCs cover device get/set, arm/gripper MIT-mode setup,
arm mode, fault clear, emergency stop, and control-lease acquire/release. The
firmware normally enables the generated RPC server and the host the client;
both call the generated nonblocking poll/deadline APIs. Emergency stop remains
available without lease ownership, while both real-time command streams require
the current token.

All messages have static encoded-size bounds under WLC ABI 8. The principal
scratch maxima are `DualDeviceInfo=112`, `DualArmStatus=421`, `TriggerEvent=13`,
`DualJointMitCommand=279`, and `DualGripperMitCommand=79` bytes; the largest RPC
payload is the 126-byte `GetDualDeviceInfoResponse`.

Reviewed `fnv1a64-v1` identities are:

| Contract/profile | Identity |
| --- | --- |
| dual schema revision 1 | `0xe38e4cc38cf301a1` |
| firmware profile | `0x11b90f15a15dbbb6` |
| host profile | `0x81855ebbc7769f79` |

The runtime is fully caller-owned. Firmware requirements include two LATEST
mailboxes; host requirements include one LATEST mailbox and
`trigger_event_fifo_capacity` entries. Selected RPC client slots or server
pending/cache slots and their response/canonical-request buffers add to the
size returned by `fci_dual_runtime_requirements()`. The caller provides storage
with the reported size/alignment and owns scheduling; no generated component
allocates memory or starts a thread.
