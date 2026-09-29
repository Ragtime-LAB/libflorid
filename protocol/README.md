# fci_protocol

This repository holds the Wirelink FCI contracts shared by host and firmware.

Current scope:
- independent single-arm, upgrade, and dual-arm Wirelink schemas
- a composed device endpoint with underwater mode/status/settings management and ten-motor Control telemetry
- host and firmware binding profiles for every component
- generated allocation-free C11 codecs, dispatchers, and runtimes

The `schema/wirelink/upgrade/fci_upgrade.wl` contract uses
typed RPC for those controls and the Wirelink constant-memory bulk state
machines for bounded image upload.

The independent `schema/wirelink/dual/fci_dual.wl` contract covers the frozen
dual product's 12 joints, two grippers, hardware trigger stream, MIT control,
typed administration, and expiring control lease without changing single-arm
array counts or identities.

Zephyr module support:
- `zephyr/module.yml` exposes this repository as a standard Zephyr module
- `CONFIG_FCI_PROTOCOL` enables the module without selecting a component
- `CONFIG_FCI_PROTOCOL_ARM`, `CONFIG_FCI_PROTOCOL_UPGRADE`, and
  `CONFIG_FCI_PROTOCOL_DUAL` select generated firmware components
- `CONFIG_FCI_PROTOCOL_DEVICE` selects the composed arm/upgrade/underwater
  endpoint; products register only the handlers they implement
- root `CMakeLists.txt` detects `ZEPHYR_BASE` and dispatches to
  `cmake/Zephyr.cmake`; otherwise it behaves like a normal standalone CMake
  subproject

Recommended Zephyr integration boundary:
- this library owns schemas and generated Wirelink bindings
- the Zephyr app owns USB CDC I/O, protocol mode switching, upgrade policy,
  `UpgradeManager`, slot writes, and reboot policy
- the application explicitly links each component it consumes, for example
  `target_link_libraries(app PRIVATE fci_protocol::arm)`; the module never
  injects all generated runtimes or include directories into `app`

The generated component targets are the only FCI implementation. The former
hand-written compatibility headers and vendored transport have been removed;
there is no alternate backend or fallback in this branch.

The desktop SDK also delegates physical ownership to Wirelink adapters.
Vendor Bulk, Astrial serial/CDC, and Asio UDP bind directly to the endpoint's
`wl_ctx_t`; RX no longer crosses a libflorid callback and FIFO-copy path.
libflorid retains URI parsing, product defaults, and device selection, while
Wirelink owns sink completion, RX borrowing, backpressure, wakeups, service,
quiescence, and adapter deadline policy.

## Wirelink schema generation

The first canonical single-arm core contract is
`schema/wirelink/arm/fci_arm.wl`. Firmware and host policies are separate:

- `firmware.bind.wl` retains joint, Cartesian, and gripper commands as
  allocation-free `LATEST` values.
- `host.bind.wl` retains arm status, diagnostics, and fixed motor feedback as
  `LATEST` values.
- `services.bind.wl` is composed with either role and defines all 19 device,
  settings, mode, home, fault, motor, emergency-stop and control-lease RPCs
  once. The role files declare COBS, client/server capability and outbound
  `send` routes. There is no generic `USBAck` in the Wirelink schema.

The current composed endpoint requires **WLC v0.8.0 / codegen ABI 32**
and its matching Wirelink runtime. The pinned runtime's CMake integration can
automatically obtain that released WLC; no development executable is required.
Deploy host and firmware together: revision 7 arm RPC
payloads are not compatible, and no legacy fallback is provided. Telemetry and
real-time command fields/IDs remain unchanged. See
[the migration guide](docs/arm-managed-endpoints.md) for ownership and build details.

Schema revision 6 extends the intentional pre-release ABI-8 baseline with an
immutable bounded device serial and transactional settings confirmation.
Device names and serials are borrowed `string<31>` views whose UTF-8 byte
length is enforced by the codec; diagnostic bus state and error counters use
exact `uint8`/`uint16` storage. Joint/register IDs and six/seven-bit masks
remain exact `uint8` fields, and the strict required-field behavior introduced
in revision 3 is unchanged.
Missing required business parameters, telemetry fields, or real-time values are
codec errors. In revision 8, call IDs/session/status live in managed metadata,
not in business DTOs; a rejection has no response body.

This feature branch treats Wirelink as the direct FCI implementation; it does
not define a parallel RPL contract or fallback. Desktop builds generate the
arm component by default. With an installed Wirelink package:

```sh
cmake -S . -B build \
  -DFCI_PROTOCOL_WIRELINK_ROLE=host \
  -DWLC_EXECUTABLE=/path/to/wlc
cmake --build build --target fci_protocol_wirelink_arm
```

If Wirelink is not installed, pass
`-DFCI_PROTOCOL_WIRELINK_SOURCE_DIR=/path/to/wirelink`. Set
`FCI_PROTOCOL_WIRELINK_COMPONENTS` to a semicolon-separated subset of `arm`,
`upgrade`, `dual`, and `device`. The generated targets are
`fci_protocol_wirelink_<component>` with aliases
`fci_protocol::<component>`.

Downstream CMake code may also make repeatable, role-specific calls:

```cmake
fci_protocol_generate_wirelink_component(
  COMPONENT upgrade
  ROLE host
  TARGET product_upgrade_protocol
  ALIAS product::upgrade_protocol)
```

`fci_protocol_enable_wirelink()` generates the configured component set.

Enable `FCI_PROTOCOL_BUILD_SCHEMA_TESTS` to compile and link the selected
generated runtime, exercise required-field encode/decode failures and static
size bounds in C, and compile all four components in both roles as strict C11
and C++20. WLC is always a host executable, including during Zephyr cross
builds. It is found on the host `PATH`, through
`WIRELINK_WLC_EXECUTABLE`, or through the explicit `WLC_EXECUTABLE` cache path
above.

## Underwater management

`schema/wirelink/device/fci_device.wl` imports
`underwater/fci_underwater.wl`; the device CMake recipe composes the shared
`underwater/services.bind.wl` with both host and firmware roles. The standalone
arm and dual contracts keep their identities and array dimensions.

| Contract | ID | Payload |
|---|---|---|
| UnderwaterMode | `0x7500` | Damp=0, Gravity=1, Teleop=2, Usb=3, Zeroing=4, Retract=5 |
| UnderwaterResult | `0x7501` | OK=0, InvalidMode=1, InvalidState=2, Busy=3, Rejected=4, InternalError=5 |
| UnderwaterSettingsResult | `0x7502` | OK=0, Busy=1, StorageFailed=2, InternalError=3 |
| UnderwaterSettingsSource | `0x7503` | Defaults=0, Stored=1, Corrupt=2, StorageError=3 |
| UnderwaterStatus | `0x7510` | mode, uptime, arm/motor masks, upgrading, bounded reason |
| UnderwaterSettings | `0x7511` | 110 tuning floats in fixed arrays/scalars |
| UnderwaterSettingsSnapshot | `0x7512` | settings, revision, source, storage_error |
| GetUnderwaterStatus | `0x7520` / `0x7521` | empty request; required result + info |
| SetUnderwaterMode | `0x7522` / `0x7523` | required mode; required result + info |
| GetUnderwaterSettings | `0x7524` / `0x7525` | empty request; required result + snapshot + reason |
| SetUnderwaterSettings | `0x7526` / `0x7527` | required settings; required result + snapshot + reason |
| GetUnderwaterDefaultSettings | `0x7528` / `0x7529` | empty request; required result + snapshot + reason |

Mode/status calls use managed RPC v2 without application operation/session IDs.
Business results are an explicit `result` field so a Busy/InvalidMode reply can
still carry its state snapshot and `reason`. Handlers return managed success
for such replies; a runtime/transport rejection is a separate failed call and
does not carry a business response. This distinction matters when mapping the
result to SDK exceptions or diagnostics.

Remote mode selection supports Damp, Gravity and Teleop. The other modes are
reported as state. A successful mode response describes the control owner's
state after processing that request; it does not wait for all motors to enable
or for Retract to align the arms. Hosts use GetUnderwaterStatus to observe
progress. `enabling_arms` and `gravity_arms` use bit 0 for master and bit 1 for
slave. Each motor mask uses bits 0..3 for joint IDs 1..4 and bit 4 for gripper
ID 5. Availability denotes discovered motors with fresh position feedback,
independent of whether they are enabled. `reason` is at most 95 UTF-8 bytes.
The optional `master_disabled_mask` and `slave_disabled_mask` are positive
acknowledgements from fresh motor feedback with status zero. Missing fields
on older firmware mean unknown; an unset enabled bit alone is not a disable
acknowledgement.

Settings include Teleop scale/offset/Kp/Kd (five axes), gravity scales (four
master then four slave joints), six friction parameter arrays (five master
then five slave motors), force feedback high/low gains, load thresholds and
velocity/load filter times. Retract trajectory limits and identified self-load
coefficients are outside this writable object. Every field is required, and
fixed packed arrays retain signed float32 values without schema-level tuning
limits or implicit substitutions.

SetUnderwaterSettings saves to persistent storage, then applies the owned copy
on the control thread before returning success. It introduces no mode guard;
upgrade/reboot ownership returns Busy. A storage failure preserves the active
settings and returns StorageFailed with a concrete errno/reason. Sources
distinguish stored values, compiled defaults, corrupt saved data and storage
errors. GetUnderwaterDefaultSettings only reads defaults; hosts explicitly set
them to reset saved settings. Managed replay does not write storage twice.
An already persisted write still applies after disconnect or an abandoned RPC;
the abandoned response does not undo persistence. A host timeout/local cancel
therefore has an unknown remote result, and hosts query settings to reconcile.

The ordinary C endpoint uses owned `underwater_status_value_t`,
`get_underwater_status_response_value_t`, and
`set_underwater_mode_response_value_t`. Initialize every required `has_*` flag;
owned strings expose `data` plus `length`. Async host entry points are
`fci_device_endpoint_get_underwater_status_async` and
`fci_device_endpoint_set_underwater_mode_async`. Firmware config callbacks are
`on_get_underwater_status` and `on_set_underwater_mode`.

`fci_protocol_underwater_endpoints` runs both generated endpoints over an
in-memory COBS link. It checks successful mode acceptance before enable
completion, Busy/InvalidMode with complete snapshots, owned response lifetimes,
mask widths, required fields, the string bound, settings ownership,
read-only defaults, upgrade Busy and the full settings response size (including
its 20-byte managed header) within a 2048-byte payload. The existing compiler
matrix also builds the device composition as strict C11 and C++20 in both roles.
