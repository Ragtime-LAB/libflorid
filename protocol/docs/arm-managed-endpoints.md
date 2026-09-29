# Arm revision 8: managed endpoint integration

Arm RPC now uses Wirelink managed metadata v2. `services.bind.wl` only pairs
business requests and responses; delivery defaults to reliable. The 19 business
RPCs no longer define `operation_id` or `status` fields. Domain status enums
retain their values: return zero plus a response on success, or a nonzero
rejection code with no response body. Telemetry and real-time commands retain
their revision-7 field numbers, IDs and encoding.

## Deployment and build

Update libflorid and all arm firmware together. Old arm RPC payloads are rejected;
there is no negotiation or mapped-format fallback. Upgrade and dual component
wire formats are unchanged, although all generated code must use the same ABI.
Use WLC **v0.6.0 / ABI 31** with Wirelink **v0.6.0** and regenerate snapshots.
WLC v0.5.0 produces ABI 30 and cannot generate this API. Download the matching
release binary or build the v0.6.0 WLC checkout with `cargo build --release --locked`.
Pass `WLC_EXECUTABLE` (desktop) or `WIRELINK_WLC_EXECUTABLE` (Zephyr) explicitly
for offline/reproducible tool selection; Wirelink also provides verified source
bootstrap when a matching compiler is not installed. The integration is on main.

`fci_protocol_configure_arm_endpoint()` sets eight RPC slots and a 4096-byte
COBS receive FIFO on the runtime CMake target. These PUBLIC definitions also
apply to snapshot consumers. Keep layout configuration here, not as macros
scattered through business headers. All translation units must agree.

## What belongs where

- Generated endpoint: clocks/session creation, buffers, dispatch, bounded owned
  request/response values, automatic client completion and slot recycling,
  server replay, cancellation and retirement of old-peer TX handles.
- libflorid: serialized business operations, a reserved lease-renewal job,
  SDK-owned result snapshots and condition-variable waits. SDK request IDs are
  local job handles, not wire call IDs. Timeout includes time spent in its queue.
- ArmProtocol: lease expiry before RX, product mode/settings validation, and
  five deferred operations (Home plus four motor operations). Their typed tokens
  are copied; cross-thread completions are queued back to the sole owner.
  A peer restart invalidates old product tokens and queued completions too.

Immediate firmware RPCs use `config.on_<rpc>` and return business status. They
do not enter the product's deferred-operation table or completion switch. To add
an immediate RPC, add its schema/profile pair, one handler and one registration;
add an SDK wrapper/request conversion if it is part of the SDK API. Only a real
slow operation needs token storage, a product deadline and terminal completion.

## Tests

`tests/support/managed_rpc_wire.hpp` is an explicit **test-only** raw metadata
fixture, shared by SDK and firmware fault-injection tests. It permits duplicate
IDs, peer restarts, malformed inputs and body-free rejections. It is not an
application API or an old-format compatibility shim. Ordinary code includes
the generated endpoint header; deferred firmware integration opts into the
advanced header.
