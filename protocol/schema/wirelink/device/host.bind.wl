profile version 1;

send BulkBegin { delivery = reliable; }
send BulkChunk { delivery = unreliable; }
send BulkEnd { delivery = reliable; }
send BulkAbort { delivery = reliable; }
direct BulkStatus { delivery = unreliable; }

// Underwater ten-axis control and state streams.
send UnderwaterArmCommand { delivery = unreliable; }
latest UnderwaterArmStatus { delivery = unreliable; }
