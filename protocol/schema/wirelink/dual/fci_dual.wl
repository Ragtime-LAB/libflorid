version 1;

// Dual-arm v1 allocation map:
//   0x7200-0x721f  enums and shared values
//   0x7300-0x731f  firmware telemetry
//   0x7380-0x73bf  typed RPC
//   0x7400-0x741f  host real-time control

enum DualFirmwareType = 29185 { // 0x7201
  DUAL_FIRMWARE_STANDARD = 0;
  DUAL_FIRMWARE_MOBILE = 1;
  DUAL_FIRMWARE_COBOT = 2;
}

// The frozen dual product supports only these modes and MIT control. Future
// enum additions remain compatible without advertising unimplemented modes in
// the v1 baseline.
enum DualArmMode = 29186 { // 0x7202
  DUAL_ARM_MODE_PC = 0;
  DUAL_ARM_MODE_DRAG = 1;
  DUAL_ARM_MODE_DAMP = 2;
}

enum DualMotorControlMode = 29187 { // 0x7203
  DUAL_MOTOR_CONTROL_MIT = 1;
}

enum DualDeviceInfoStatus = 29188 { // 0x7204
  DUAL_DEVICE_INFO_OK = 0;
  DUAL_DEVICE_INFO_STORAGE_FAILED = 1;
  DUAL_DEVICE_INFO_NAME_TOO_LONG = 2;
  DUAL_DEVICE_INFO_INVALID_ARGUMENT = 3;
  DUAL_DEVICE_INFO_BUSY = 4;
  DUAL_DEVICE_INFO_INTERNAL_ERROR = 5;
}

enum DualModeStatus = 29189 { // 0x7205
  DUAL_MODE_OK = 0;
  DUAL_MODE_INVALID_MODE = 1;
  DUAL_MODE_INVALID_STATE = 2;
  DUAL_MODE_BUSY = 3;
  DUAL_MODE_REJECTED = 4;
  DUAL_MODE_INTERNAL_ERROR = 5;
}

enum DualFaultStatus = 29190 { // 0x7206
  DUAL_FAULT_OK = 0;
  DUAL_FAULT_INVALID_STATE = 1;
  DUAL_FAULT_BUSY = 2;
  DUAL_FAULT_TIMED_OUT = 3;
  DUAL_FAULT_FAILED = 4;
  DUAL_FAULT_UNSUPPORTED = 5;
}

enum DualEmergencyStopStatus = 29191 { // 0x7207
  DUAL_EMERGENCY_STOP_OK = 0;
  DUAL_EMERGENCY_STOP_ALREADY_STOPPED = 1;
  DUAL_EMERGENCY_STOP_REJECTED = 2;
  DUAL_EMERGENCY_STOP_INTERNAL_ERROR = 3;
}

enum DualControlLeaseStatus = 29192 { // 0x7208
  DUAL_CONTROL_LEASE_OK = 0;
  DUAL_CONTROL_LEASE_BUSY = 1;
  DUAL_CONTROL_LEASE_INVALID_TIMEOUT = 2;
  DUAL_CONTROL_LEASE_INVALID_TOKEN = 3;
  DUAL_CONTROL_LEASE_EXPIRED = 4;
  DUAL_CONTROL_LEASE_NOT_HELD = 5;
  DUAL_CONTROL_LEASE_INTERNAL_ERROR = 6;
}

message DualSemanticVersion = 29201 { // 0x7211
  required uint32 major = 1;
  required uint32 minor = 2;
  required uint32 patch = 3;
}

message DualDeviceInfo = 29202 { // 0x7212
  required DualSemanticVersion protocol_version = 1;
  required DualSemanticVersion firmware_version = 2;
  required string<31> board_name = 3;
  required string<31> custom_name = 4;
  required DualFirmwareType firmware_type = 5;
}

// Joint indices 0..5 are arm 1 and 6..11 arm 2. Gripper indices 0 and 1
// correspond to arms 1 and 2. Per-arm vectors are concatenated in that order.
message DualArmStatus = 29441 { // 0x7301
  required DualArmMode mode = 1;
  required fixed32 sequence = 2;
  required fixed64 timestamp_us = 3;
  required packed float32 joint_position[12] = 4;
  required packed float32 joint_velocity[12] = 5;
  required packed float32 joint_torque[12] = 6;
  required packed float32 base_gravity[6] = 7;
  required packed float32 gripper_position[2] = 8;
  required packed float32 gripper_velocity[2] = 9;
  required packed float32 gripper_torque[2] = 10;
  required packed float32 end_effector_transform[32] = 11;
  required packed float32 external_wrench[12] = 12;
  required fixed32 error_flags = 13;
  required fixed64 last_sdk_timestamp_us = 14;
}

message TriggerEvent = 29442 { // 0x7302
  required fixed64 timestamp_us = 1;
  required uint16 sequence = 2;
}

message GetDualDeviceInfoRequest = 29568 { // 0x7380
  required uint32 operation_id = 1;
}

message GetDualDeviceInfoResponse = 29569 { // 0x7381
  required uint32 operation_id = 1;
  required DualDeviceInfoStatus status = 2;
  optional DualDeviceInfo info = 3;
}

message SetDualDeviceInfoRequest = 29570 { // 0x7382
  required uint32 operation_id = 1;
  required string<31> custom_name = 2;
  required DualFirmwareType firmware_type = 3;
}

message SetDualDeviceInfoResponse = 29571 { // 0x7383
  required uint32 operation_id = 1;
  required DualDeviceInfoStatus status = 2;
}

message SetDualArmControlModeRequest = 29572 { // 0x7384
  required uint32 operation_id = 1;
  required DualMotorControlMode mode = 2;
}

message SetDualArmControlModeResponse = 29573 { // 0x7385
  required uint32 operation_id = 1;
  required DualModeStatus status = 2;
}

message SetDualGripperControlModeRequest = 29574 { // 0x7386
  required uint32 operation_id = 1;
  required DualMotorControlMode mode = 2;
}

message SetDualGripperControlModeResponse = 29575 { // 0x7387
  required uint32 operation_id = 1;
  required DualModeStatus status = 2;
}

message SetDualArmModeRequest = 29576 { // 0x7388
  required uint32 operation_id = 1;
  required DualArmMode mode = 2;
}

message SetDualArmModeResponse = 29577 { // 0x7389
  required uint32 operation_id = 1;
  required DualModeStatus status = 2;
}

message ClearDualFaultsRequest = 29578 { // 0x738a
  required uint32 operation_id = 1;
}

message ClearDualFaultsResponse = 29579 { // 0x738b
  required uint32 operation_id = 1;
  required DualFaultStatus status = 2;
}

message DualEmergencyStopRequest = 29580 { // 0x738c
  required uint32 operation_id = 1;
}

message DualEmergencyStopResponse = 29581 { // 0x738d
  required uint32 operation_id = 1;
  required DualEmergencyStopStatus status = 2;
}

message AcquireDualControlLeaseRequest = 29582 { // 0x738e
  required uint32 operation_id = 1;
  required fixed32 requested_timeout_ms = 2;
  optional fixed64 current_token = 3;
}

message AcquireDualControlLeaseResponse = 29583 { // 0x738f
  required uint32 operation_id = 1;
  required DualControlLeaseStatus status = 2;
  optional fixed64 lease_token = 3;
  optional fixed32 granted_timeout_ms = 4;
}

message ReleaseDualControlLeaseRequest = 29584 { // 0x7390
  required uint32 operation_id = 1;
  required fixed64 lease_token = 2;
}

message ReleaseDualControlLeaseResponse = 29585 { // 0x7391
  required uint32 operation_id = 1;
  required DualControlLeaseStatus status = 2;
}

// Both real-time streams require the active lease token. Sequence remains the
// frozen product's uint16 counter; dt and gravity are explicit Wirelink fields
// rather than bits hidden in a legacy control-mode byte.
message DualJointMitCommand = 29697 { // 0x7401
  required packed float32 position[12] = 1;
  required packed float32 velocity[12] = 2;
  required packed float32 torque[12] = 3;
  required packed float32 kp[12] = 4;
  required packed float32 kd[12] = 5;
  required fixed32 dt_us = 6;
  required uint16 sequence = 7;
  required bool gravity_compensation = 8;
  required fixed64 sdk_timestamp_us = 9;
  required fixed64 lease_token = 10;
}

message DualGripperMitCommand = 29698 { // 0x7402
  required packed float32 position[2] = 1;
  required packed float32 velocity[2] = 2;
  required packed float32 torque[2] = 3;
  required packed float32 kp[2] = 4;
  required packed float32 kd[2] = 5;
  required fixed32 dt_us = 6;
  required uint16 sequence = 7;
  required bool gravity_compensation = 8;
  required fixed64 sdk_timestamp_us = 9;
  required fixed64 lease_token = 10;
}
