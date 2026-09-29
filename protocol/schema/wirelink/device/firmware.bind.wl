profile version 1;

// Bulk callbacks borrow decoded payloads until return. No latest mailbox or
// RPC slot is allocated for a data chunk.
direct BulkBegin { delivery = reliable; }
direct BulkChunk { delivery = unreliable; }
direct BulkEnd { delivery = reliable; }
direct BulkAbort { delivery = reliable; }
send BulkStatus { delivery = unreliable; }

// Underwater ten-axis control and state streams.
latest UnderwaterArmCommand { delivery = unreliable; }
send UnderwaterArmStatus { delivery = unreliable; }
