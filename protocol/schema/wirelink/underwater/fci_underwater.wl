version 2;

// Underwater product management. These contracts are composed into fci_device;
// they do not change the independent six-axis arm or twelve-axis dual schemas.
// Allocation: 0x7500..0x751f values; 0x7520..0x752f managed RPC messages.
enum UnderwaterMode = 29952 { // 0x7500
  UNDERWATER_MODE_DAMP = 0;
  UNDERWATER_MODE_GRAVITY = 1;
  UNDERWATER_MODE_TELEOP = 2;
  UNDERWATER_MODE_CONTROL = 3;
  UNDERWATER_MODE_ZEROING = 4;
  UNDERWATER_MODE_RETRACT = 5;
}

// Business results accompany the state snapshot even when a mode request
// cannot be accepted. Managed RPC rejection remains a runtime/protocol error;
// business handlers return managed success and put their result here.
enum UnderwaterResult = 29953 { // 0x7501
  UNDERWATER_RESULT_OK = 0;
  UNDERWATER_RESULT_INVALID_MODE = 1;
  UNDERWATER_RESULT_INVALID_STATE = 2;
  UNDERWATER_RESULT_BUSY = 3;
  UNDERWATER_RESULT_REJECTED = 4;
  UNDERWATER_RESULT_INTERNAL_ERROR = 5;
}

enum UnderwaterSettingsResult = 29954 { // 0x7502
  UNDERWATER_SETTINGS_OK = 0;
  UNDERWATER_SETTINGS_BUSY = 1;
  UNDERWATER_SETTINGS_STORAGE_FAILED = 2;
  UNDERWATER_SETTINGS_INTERNAL_ERROR = 3;
}

enum UnderwaterSettingsSource = 29955 { // 0x7503
  UNDERWATER_SETTINGS_SOURCE_DEFAULTS = 0;
  UNDERWATER_SETTINGS_SOURCE_STORED = 1;
  UNDERWATER_SETTINGS_SOURCE_CORRUPT = 2;
  UNDERWATER_SETTINGS_SOURCE_STORAGE_ERROR = 3;
}

message UnderwaterStatus = 29968 { // 0x7510
  required UnderwaterMode mode = 1;
  required fixed32 uptime_ms = 2;
  // Arm masks: bit 0 master, bit 1 slave.
  required uint8 enabling_arms = 3;
  required uint8 gravity_arms = 4;
  // Motor masks: bits 0..3 joints 1..4, bit 4 gripper (motor ID 5).
  // Available means discovered with fresh position feedback; enabled state is
  // reported separately and is not a prerequisite for availability.
  required uint8 master_available_mask = 5;
  required uint8 slave_available_mask = 6;
  required uint8 master_enabled_mask = 7;
  required uint8 slave_enabled_mask = 8;
  required uint8 master_disable_pending_mask = 9;
  required uint8 slave_disable_pending_mask = 10;
  required bool upgrading = 11;
  // Current mode/transition detail, including pending enables or missing
  // feedback. UTF-8 byte limit; not a separate high-rate telemetry stream.
  required string<95> reason = 12;
  // Newer firmware reports fresh feedback explicitly confirming status=0.
  // Unlike "not enabled", these bits are positive disable acknowledgements.
  optional uint8 master_disabled_mask = 13;
  optional uint8 slave_disabled_mask = 14;
  optional fixed64 control_session = 15;
}

// Axis order: vectors of 5 use joints 1..4, then the gripper. Gravity uses
// master joints 1..4 followed by slave joints 1..4. Friction vectors use all
// five master motors followed by all five slave motors. Units are SI.
// This tuning object excludes Retract trajectory limits, zero calibration,
// and identified self-load coefficients. No raw native struct is on the wire.
message UnderwaterSettings = 29969 { // 0x7511
  required packed float32 angle_scale[5] = 1;
  required packed float32 angle_offset_rad[5] = 2;
  required packed float32 kp[5] = 3;
  required packed float32 kd[5] = 4;
  required packed float32 gravity_scale[8] = 5;
  required packed float32 friction_coulomb_positive_nm[10] = 6;
  required packed float32 friction_coulomb_negative_nm[10] = 7;
  required packed float32 friction_viscous_nm_per_rad_s[10] = 8;
  required packed float32 friction_deadband_rad_s[10] = 9;
  required packed float32 friction_transition_rad_s[10] = 10;
  required packed float32 friction_filter_time_constant_s[10] = 11;
  required packed float32 force_gain[5] = 12;
  required packed float32 force_low_gain[5] = 13;
  required float32 force_low_load_nm = 14;
  required float32 force_high_load_nm = 15;
  required packed float32 force_velocity_filter_s[5] = 16;
  required packed float32 force_load_filter_s[5] = 17;
  // Missing on writes preserves the current alarm preference (older SDKs).
  // New firmware includes this field in reads and compiled defaults.
  optional bool buzzer_alarm_enabled = 18;
}

message UnderwaterSettingsSnapshot = 29970 { // 0x7512
  required UnderwaterSettings settings = 1;
  required fixed32 revision = 2;
  required UnderwaterSettingsSource source = 3;
  required int32 storage_error = 4;
}

message GetUnderwaterStatusRequest = 29984 { // 0x7520
}

message GetUnderwaterStatusResponse = 29985 { // 0x7521
  required UnderwaterResult result = 1;
  required UnderwaterStatus info = 2;
}

message SetUnderwaterModeRequest = 29986 { // 0x7522
  // Damp, Gravity, Teleop and Control are remotely selectable.
  // Zero calibration uses Zeroing, not SetUnderwaterMode.
  required UnderwaterMode mode = 1;
}

message SetUnderwaterModeResponse = 29987 { // 0x7523
  required UnderwaterResult result = 1;
  // Snapshot after the control owner processes this request. Acceptance does
  // not imply all motors have completed enabling or Teleop alignment.
  required UnderwaterStatus info = 2;
}

message GetUnderwaterSettingsRequest = 29988 { // 0x7524
}

message GetUnderwaterSettingsResponse = 29989 { // 0x7525
  required UnderwaterSettingsResult result = 1;
  required UnderwaterSettingsSnapshot snapshot = 2;
  required string<95> reason = 3;
}

message SetUnderwaterSettingsRequest = 29990 { // 0x7526
  required UnderwaterSettings settings = 1;
}

message SetUnderwaterSettingsResponse = 29991 { // 0x7527
  required UnderwaterSettingsResult result = 1;
  // Success acknowledges persistent storage and application by the control
  // owner. A failed write retains the active snapshot and a concrete reason.
  required UnderwaterSettingsSnapshot snapshot = 2;
  required string<95> reason = 3;
}

message GetUnderwaterDefaultSettingsRequest = 29992 { // 0x7528
}

message GetUnderwaterDefaultSettingsResponse = 29993 { // 0x7529
  required UnderwaterSettingsResult result = 1;
  // Read-only firmware defaults; does not reset active or persisted settings.
  required UnderwaterSettingsSnapshot snapshot = 2;
  required string<95> reason = 3;
}

// Ten motors: master joints 1..4, master gripper, slave joints 1..4, slave gripper.
// Position/velocity/torque are motor coordinates, in rad, rad/s, Nm.
enum UnderwaterControlKind = 29956 {
  UNDERWATER_CONTROL_MIT = 1;
  UNDERWATER_CONTROL_MIT_WITH_GRAVITY = 2;
}

message UnderwaterArmCommand = 29972 { // 0x7514
  required packed float32 q[10] = 1;
  required packed float32 dq[10] = 2;
  required packed float32 kp[10] = 3;
  required packed float32 kd[10] = 4;
  required packed float32 tau[10] = 5;
  required UnderwaterControlKind kind = 6;
  required fixed32 sequence = 7;
  required fixed64 sdk_timestamp_us = 8;
  required fixed64 control_session = 9;
}

message UnderwaterArmStatus = 29971 { // 0x7513
  required packed float32 q[10] = 1;
  required packed float32 dq[10] = 2;
  required packed float32 tau[10] = 3;
  // Bits 0..7 temperature (C), bits 8..15 raw DM status, upper bits reserved.
  required packed fixed32 motor_info[10] = 4;
  // UINT32_MAX means no feedback has ever been observed.
  required packed fixed32 feedback_age_ms[10] = 6;
  required uint16 fresh_mask = 7;
  required uint16 enabled_mask = 8;
  required uint16 available_mask = 9;
  required UnderwaterMode mode = 10;
  required UnderwaterControlKind kind = 11;
  required fixed32 sequence = 12;
  required fixed64 timestamp_us = 13;
  required fixed64 control_session = 14;
  // CAN queue acceptance, not an acknowledgement from the motor drive.
  required fixed32 applied_command_sequence = 15;
  required fixed64 last_sdk_timestamp_us = 16;
  required uint16 applied_mask = 17;
  required bool command_received = 18;
  // 0 none, 1 command timeout, 2 USB disconnect.
  required uint8 control_stop_reason = 19;
  required fixed32 commands_received = 20;
  required fixed32 commands_overwritten = 21;
  required fixed32 commands_rejected = 22;
  // 0 none, 1 not Control, 2 session, 3 sequence, 4 kind, 5 numeric fields, 6 stale.
  required uint8 command_reject_reason = 23;
}

// Set the current pose as zero on every configured motor. Requires Damp.
// Completion waits for the existing post-zero feedback/retry sequence.
message ZeroingRequest = 29994 { // 0x752a
}
message ZeroingResponse = 29995 { // 0x752b
  required UnderwaterResult result = 1;
  required UnderwaterStatus info = 2;
}
