version 1;
// Additive four-motor product API; UART retains its independent Protocol 3 framing.
message SingleCommand = 30211 { // 0x7514
  required packed float32 q[4] = 1;
  required packed float32 dq[4] = 2;
  required packed float32 kp[4] = 3;
  required packed float32 kd[4] = 4;
  required packed float32 tau[4] = 5;
  required UnderwaterControlKind kind = 6;
  required fixed32 sequence = 7;
  required fixed64 sdk_timestamp_us = 8;
  required fixed64 control_session = 9;
}

message SingleState = 30212 { // 0x7513
  required packed float32 q[4] = 1;
  required packed float32 dq[4] = 2;
  required packed float32 tau[4] = 3;
  // Bits 0..7 temperature (C), bits 8..15 raw DM status, upper bits reserved.
  required packed fixed32 motor_info[4] = 4;
  // UINT32_MAX means no feedback has ever been observed.
  required packed fixed32 feedback_age_ms[4] = 6;
  required uint16 fresh_mask = 7;
  required uint16 enabled_mask = 8;
  required uint16 available_mask = 9;
  required uint8 mode = 10;
  required UnderwaterControlKind kind = 11;
  required fixed32 sequence = 12;
  required fixed64 timestamp_us = 13;
  required fixed64 control_session = 14;
  // CAN queue acceptance, not an acknowledgement from the motor drive.
  required fixed32 applied_command_sequence = 15;
  required fixed64 last_sdk_timestamp_us = 16;
  required uint16 applied_mask = 17;
  required bool command_received = 18;
  // 0 none, 1 timeout, 3 output failure, 4 enable failure.
  required uint8 control_stop_reason = 19;
  required fixed32 commands_received = 20;
  required fixed32 commands_overwritten = 21;
  required fixed32 commands_rejected = 22;
  // 0 none, 1 not Control, 2 session, 3 sequence, 4 kind, 5 numeric fields, 6 stale.
  required uint8 command_reject_reason = 23;
  required uint8 control_source = 24;
  required bool enabling = 25;
  required bool upgrading = 26;
}


message SingleRequest = 30209 {
  // 0 info/status, 1 mode, 2 read gravity settings, 3 persist gravity settings.
  required uint8 operation = 1;
  required uint8 mode = 2;
  required packed float32 gravity_scale[3] = 3;
}
message SingleResponse = 30210 {
  required uint8 result = 1;
  required SingleState state = 2;
  required fixed64 boot_id = 3;
  required string<32> uid = 4;
  required uint8 role = 5;
  required packed float32 gravity_scale[3] = 6;
  required fixed32 revision = 7;
  required int32 storage_error = 8;
  required uint16 disable_pending_mask = 9;
  required uint16 disabled_mask = 10;
  required string<95> reason = 11;
}
