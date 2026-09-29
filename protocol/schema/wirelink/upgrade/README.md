# Upgrade contract

`fci_upgrade.wl` is the complete Wirelink v1 contract for upgrade control and
firmware-image upload. It replaces the legacy RPL request/ack packets; it is
not a compatibility wrapper.

The three control operations are typed RPCs. Image bytes use Wirelink's
constant-memory bulk sender/receiver state machines and the five `Bulk*`
messages in this directory. `BulkChunk.data` is a borrowed view, so firmware
must pass it synchronously to flash/storage before releasing the RX event.

Normal arm traffic and upgrade traffic use an explicit product-owned mode
switch. Implementations must never infer the mode by sniffing the first byte.
For USB vendor bulk and UDP the framing profile is `COBS_STREAM + NONE`; the
whole-image CRC32C in `StartUpgrade`/`BulkBegin` remains mandatory because it
validates durable object contents rather than an individual transport frame.

The generated host/firmware profiles retain only RPC state. Bulk messages are
decoded by the terminal event dispatcher and fed directly to
`wl_bulk_sender_t` or `wl_bulk_receiver_t`, preserving the RX borrow lifetime.
They use unreliable Wirelink DATA: the bulk phase/status protocol supplies its
own idempotence, ordering, timeout, and bounded retry policy.

`BulkChunk.data` is `bytes<2031>`: its worst-case 17-byte message metadata makes
`BULK_CHUNK_MAX_ENCODED_SIZE` exactly 2048 bytes, matching the current Wirelink
frame ceiling. The accepted chunk must still be the minimum of peer/configured
limits and `max_payload_len - 17`; smaller endpoint payload configurations
therefore negotiate smaller chunks. Raising the schema bound later requires an
explicit incompatible schema revision together with a Wirelink frame-limit
change.

All messages have static encoded-size bounds under WLC ABI 8. The transfer
messages are `BulkBegin=24`, `BulkChunk=2048`, `BulkEnd=19`, `BulkAbort=11`, and
`BulkStatus=31` bytes. The typed RPC maxima are `GetBootStatus` request/response
`6/90`, `StartUpgrade` `53/34`, and `Reboot` `12/18` bytes.

Reviewed `fnv1a64-v1` identities are:

| Contract/profile | Identity |
| --- | --- |
| upgrade schema revision 1 | `0xea7b026521578dd1` |
| firmware profile | `0xb19ce5d2188d0572` |
| host profile | `0xb19ce5d2188d0572` |

Firmware normally enables only the generated RPC server and host only the RPC
client; both must call the generated nonblocking poll/deadline APIs. Client
slots, server pending/cache slots, canonical request buffers, response cache,
and the 256-byte `wl_bulk_sender_t`/`wl_bulk_receiver_t` objects are all
caller-owned. No generated component allocates memory or starts a thread.
