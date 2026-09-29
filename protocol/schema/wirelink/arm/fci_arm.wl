version 8;

// Legacy IDs without standalone Wirelink messages remain reserved to prevent
// accidental reuse and preserve historical diagnostics. Gripper state is
// folded into ArmStatus and motor feedback; the generic USBAck is replaced by
// typed responses.
reserved 24579; // 0x6003 legacy standalone GripperStatus
reserved 28656; // 0x6ff0

// FCI Wirelink v1 allocation map:
//   0x5000-0x51ff  shared value and enum declarations
//   0x6000-0x60ff  firmware telemetry
//   0x6100-0x62ff  typed RPC requests and responses using legacy request IDs
//   0x6300-0x63ff  real-time host commands plus emergency-stop RPC
// WLC currently accepts decimal IDs, so the values below are the decimal
// representation of the documented hexadecimal allocations.

enum FirmwareType = 20481 { // 0x5001
  FIRMWARE_STANDARD_ARM = 0;
  FIRMWARE_MOBILE_ARM = 1;
  FIRMWARE_COBOT_ARM = 2;
}

enum ArmMode = 20482 { // 0x5002
  ARM_MODE_PC = 0;
  ARM_MODE_DRAG = 1;
  ARM_MODE_DAMP = 2;
  ARM_MODE_RETRACTING = 3;
  ARM_MODE_TELEOP = 4;
}

enum MotorControlMode = 20483 { // 0x5003
  MOTOR_CONTROL_MIT = 1;
  MOTOR_CONTROL_POSITION_VELOCITY = 2;
  MOTOR_CONTROL_VELOCITY = 3;
  MOTOR_CONTROL_PVT = 4;
}

enum DeviceInfoStatus = 20484 { // 0x5004
  DEVICE_INFO_OK = 0;
  DEVICE_INFO_STORAGE_FAILED = 1;
  DEVICE_INFO_NAME_TOO_LONG = 2;
  DEVICE_INFO_INVALID_ARGUMENT = 3;
  DEVICE_INFO_BUSY = 4;
  DEVICE_INFO_INTERNAL_ERROR = 5;
}

enum DeviceSettingsStatus = 20485 { // 0x5005
  DEVICE_SETTINGS_OK = 0;
  DEVICE_SETTINGS_STORAGE_FAILED = 1;
  DEVICE_SETTINGS_INVALID_ARGUMENT = 2;
  DEVICE_SETTINGS_BUSY = 3;
  DEVICE_SETTINGS_INTERNAL_ERROR = 4;
}

enum ModeStatus = 20486 { // 0x5006
  MODE_OK = 0;
  MODE_INVALID_MODE = 1;
  MODE_INVALID_STATE = 2;
  MODE_BUSY = 3;
  MODE_REJECTED = 4;
  MODE_INTERNAL_ERROR = 5;
}

enum HomeStatus = 20487 { // 0x5007
  HOME_OK = 0;
  HOME_BUSY = 1;
  HOME_INVALID_STATE = 2;
  HOME_FAILED = 3;
  HOME_TIMED_OUT = 4;
  HOME_CANCELLED = 5;
}

enum MotorOperationStatus = 20488 { // 0x5008
  MOTOR_OPERATION_OK = 0;
  MOTOR_OPERATION_INVALID_JOINT = 1;
  MOTOR_OPERATION_INVALID_REGISTER = 2;
  MOTOR_OPERATION_NOT_IN_DAMP_MODE = 3;
  MOTOR_OPERATION_BUSY = 4;
  MOTOR_OPERATION_TIMED_OUT = 5;
  MOTOR_OPERATION_IO_ERROR = 6;
  MOTOR_OPERATION_UNSUPPORTED = 7;
}

enum FaultOperationStatus = 20489 { // 0x5009
  FAULT_OPERATION_OK = 0;
  FAULT_OPERATION_INVALID_JOINT = 1;
  FAULT_OPERATION_INVALID_STATE = 2;
  FAULT_OPERATION_BUSY = 3;
  FAULT_OPERATION_TIMED_OUT = 4;
  FAULT_OPERATION_FAILED = 5;
  FAULT_OPERATION_UNSUPPORTED = 6;
}

enum EmergencyStopStatus = 20490 { // 0x500a
  EMERGENCY_STOP_OK = 0;
  EMERGENCY_STOP_ALREADY_STOPPED = 1;
  EMERGENCY_STOP_REJECTED = 2;
  EMERGENCY_STOP_INTERNAL_ERROR = 3;
}

enum ControlLeaseStatus = 20491 { // 0x500b
  CONTROL_LEASE_OK = 0;
  CONTROL_LEASE_BUSY = 1;
  CONTROL_LEASE_INVALID_TIMEOUT = 2;
  CONTROL_LEASE_INVALID_TOKEN = 3;
  CONTROL_LEASE_EXPIRED = 4;
  CONTROL_LEASE_NOT_HELD = 5;
  CONTROL_LEASE_INTERNAL_ERROR = 6;
}

message SemanticVersion = 20737 { // 0x5101
  required uint32 major = 1;
  required uint32 minor = 2;
  required uint32 patch = 3;
}

message DeviceInfo = 20738 { // 0x5102
  required SemanticVersion protocol_version = 1;
  required SemanticVersion firmware_version = 2;
  required string<31> board_name = 3;
  required string<31> custom_name = 4;
  required FirmwareType firmware_type = 5;
  // Immutable product identity. Firmware derives or provisions this value;
  // normal settings operations must never rewrite it.
  required string<31> serial = 6;
  // Bitmask of real-time commands consumed by this firmware image:
  // bit 0 JointMIT, 1 JointPositionVelocity, 2 JointVelocity, 3 JointPVT,
  // 4 CartesianPose, 5 CartesianVelocity, 6 GripperMIT,
  // 7 GripperPositionVelocity, 8 GripperVelocity, 9 GripperPVT. Zero means no
  // command support was advertised.
  optional fixed64 command_capabilities = 7 [default = 0];
}

// Fixed arrays keep the settings representation allocation-free and avoid the
// caller-owned repeated backing that an RPC scratch object would otherwise
// require. The seven torque entries are joints 0..5 followed by the gripper.
message DeviceSettings = 20739 { // 0x5103
  required fixed32 firmware_dt_us = 1;
  required packed float32 gravity_scale[6] = 2;
  required packed float32 torque_continuous[7] = 3;
  required packed float32 torque_peak[7] = 4;
  required packed float32 thermal_capacity[7] = 5;
  required packed float32 torque_ramp_rate[7] = 6;
  required packed float32 joint_limit_min[6] = 7;
  required packed float32 joint_limit_max[6] = 8;
}

// Array index 0..5 identifies an arm joint and index 6 the gripper. Packing
// the four float vectors avoids seven nested messages and all dynamic storage.
// Each motor consumes four bits in device_status_bits; enabled_mask uses one
// bit per motor.
message MotorFeedback = 24578 { // 0x6002
  required packed float32 position_rad[7] = 1;
  required packed float32 velocity_rad_s[7] = 2;
  required packed float32 torque_nm[7] = 3;
  required packed float32 temperature_c[7] = 4;
  required fixed32 device_status_bits = 5;
  required uint8 enabled_mask = 6;
}

// Firmware -> host telemetry. These are retained as LATEST by the host
// profile, not by the firmware profile.
message ArmStatus = 24577 { // 0x6001
  required ArmMode mode = 1;
  required fixed32 sequence = 2;
  required fixed64 timestamp_us = 3;
  required packed float32 joint_position[6] = 4;
  required packed float32 joint_velocity[6] = 5;
  required packed float32 joint_torque[6] = 6;
  required packed float32 base_gravity[3] = 7;
  required float32 gripper_position = 8;
  required float32 gripper_velocity = 9;
  required float32 gripper_torque = 10;
  required packed float32 end_effector_transform[16] = 11;
  required packed float32 external_wrench[6] = 12;
  required fixed32 error_flags = 13;
  required fixed64 last_sdk_timestamp_us = 14;
}

message ArmDiagnostics = 24580 { // 0x6004
  required fixed32 uptime_s = 1;
  required fixed32 tick_count = 2;
  required fixed32 mode_entry_ms = 3;
  required bool bus_healthy = 4;
  required uint8 bus_state = 5;
  required uint16 tx_error_count = 6;
  required uint16 rx_error_count = 7;
  required uint8 joint_healthy_mask = 8;
  required packed float32 joint_temperature_c[6] = 9;
  required bool gripper_healthy = 10;
  required float32 gripper_temperature_c = 11;
  // Bit i set while motor i (0..5 joints, 6 gripper) is in an over-temperature
  // fault (DM status MOS/Coil over-temp). Hosts must treat a non-zero mask as
  // "motion stopped / PC entry refused until it clears".
  required uint8 overheat_mask = 12;
}

// Host -> firmware real-time control. Freshness wins over history, so the
// firmware binding retains these messages in independent LATEST mailboxes.
message JointMitCommand = 25345 { // 0x6301
  required packed float32 position[6] = 1;
  required packed float32 velocity[6] = 2;
  required packed float32 torque[6] = 3;
  required packed float32 kp[6] = 4;
  required packed float32 kd[6] = 5;
  required fixed32 dt_us = 6;
  required fixed32 sequence = 7;
  required bool gravity_compensation = 8;
  required fixed64 sdk_timestamp_us = 9;
  required fixed64 lease_token = 10;
}

message GripperMitCommand = 25349 { // 0x6305
  required float32 position = 1;
  required float32 velocity = 2;
  required float32 torque = 3;
  required float32 kp = 4;
  required float32 kd = 5;
  required fixed32 dt_us = 6;
  required fixed32 sequence = 7;
  required bool gravity_compensation = 8;
  required fixed64 sdk_timestamp_us = 9;
  required fixed64 lease_token = 10;
}

message JointPositionVelocityCommand = 25350 { // 0x6306
  required packed float32 position[6] = 1;
  required packed float32 velocity[6] = 2;
  required uint8 enabled_mask = 3;
  required fixed32 sequence = 4;
  required fixed64 sdk_timestamp_us = 5;
  required fixed64 lease_token = 6;
}

message JointVelocityCommand = 25351 { // 0x6307
  required packed float32 velocity[6] = 1;
  required uint8 enabled_mask = 2;
  required fixed32 sequence = 3;
  required fixed64 sdk_timestamp_us = 4;
  required fixed64 lease_token = 5;
}

message JointPvtCommand = 25352 { // 0x6308
  required packed float32 position[6] = 1;
  required packed float32 velocity_limit[6] = 2;
  required packed float32 current_limit_normalized[6] = 3;
  required uint8 enabled_mask = 4;
  required fixed32 sequence = 5;
  required fixed64 sdk_timestamp_us = 6;
  required fixed64 lease_token = 7;
}

message CartesianPoseCommand = 25353 { // 0x6309
  required packed float32 transform[16] = 1;
  required packed float32 kp[6] = 2;
  required packed float32 kd[6] = 3;
  required fixed32 dt_us = 4;
  required fixed32 sequence = 5;
  required bool gravity_compensation = 6;
  required fixed64 sdk_timestamp_us = 7;
  required fixed64 lease_token = 8;
}

message CartesianVelocityCommand = 25354 { // 0x630a
  required packed float32 twist[6] = 1;
  required packed float32 kp[6] = 2;
  required packed float32 kd[6] = 3;
  required fixed32 dt_us = 4;
  required fixed32 sequence = 5;
  required bool gravity_compensation = 6;
  required fixed64 sdk_timestamp_us = 7;
  required fixed64 lease_token = 8;
}

message GripperPositionVelocityCommand = 25361 { // 0x6311
  required float32 position = 1;
  required float32 velocity = 2;
  required fixed32 sequence = 3;
  required fixed64 sdk_timestamp_us = 4;
  required fixed64 lease_token = 5;
}

message GripperVelocityCommand = 25362 { // 0x6312
  required float32 velocity = 1;
  required fixed32 sequence = 2;
  required fixed64 sdk_timestamp_us = 3;
  required fixed64 lease_token = 4;
}

message GripperPvtCommand = 25363 { // 0x6313
  required float32 position = 1;
  required float32 velocity_limit = 2;
  required float32 current_limit_normalized = 3;
  required fixed32 sequence = 4;
  required fixed64 sdk_timestamp_us = 5;
  required fixed64 lease_token = 6;
}

// Managed RPC: Wirelink owns call identity, session correlation and rejection
// status. Messages contain business data only; nonzero domain status rejects
// without a response body. Revision 8 requires paired host/firmware deployment.
message SetZeroRequest = 24839 { // 0x6107
  required uint8 joint_id = 2;
}

message SetZeroResponse = 24840 { // 0x6108
}

message ClearErrorRequest = 24841 { // 0x6109
  required uint8 joint_id = 2;
}

message ClearErrorResponse = 24842 { // 0x610a
}

message HomeRequest = 25089 { // 0x6201
}

message HomeResponse = 25090 { // 0x6202
}

message ClearFaultsRequest = 25093 { // 0x6205
}

message ClearFaultsResponse = 25094 { // 0x6206
}

// The old connected/disconnected notifications at 0x6209/0x620b are replaced
// by an explicit expiring lease. An absent current_token requests a new lease;
// a matching token renews it. Every real-time command carries the granted
// token so ownership remains unambiguous across multiple transports.
message AcquireControlLeaseRequest = 25097 { // 0x6209
  required fixed32 requested_timeout_ms = 2;
  optional fixed64 current_token = 3;
}

message AcquireControlLeaseResponse = 25098 { // 0x620a
  optional fixed64 lease_token = 3;
  optional fixed32 granted_timeout_ms = 4;
}

message ReleaseControlLeaseRequest = 25099 { // 0x620b
  required fixed64 lease_token = 2;
}

message ReleaseControlLeaseResponse = 25100 { // 0x620c
}

message GetMotorFeedbackRequest = 25107 { // 0x6213
}

message GetMotorFeedbackResponse = 25108 { // 0x6214
  optional MotorFeedback feedback = 3;
}

message GetDeviceInfoRequest = 25109 { // 0x6215
}

message GetDeviceInfoResponse = 25110 { // 0x6216
  optional DeviceInfo info = 3;
}

message SetDeviceInfoRequest = 25111 { // 0x6217
  required string<31> custom_name = 2;
}

message SetDeviceInfoResponse = 25112 { // 0x6218
}

message SetArmControlModeRequest = 25113 { // 0x6219
  required MotorControlMode mode = 2;
}

message SetArmControlModeResponse = 25114 { // 0x621a
}

message SetGripperControlModeRequest = 25115 { // 0x621b
  required MotorControlMode mode = 2;
}

message SetGripperControlModeResponse = 25116 { // 0x621c
}

message MotorRegisterReadRequest = 25117 { // 0x621d
  required uint8 joint_id = 2;
  required uint8 register_id = 3;
}

message MotorRegisterReadResponse = 25118 { // 0x621e
  optional uint8 joint_id = 3;
  optional uint8 register_id = 4;
  optional float32 value = 5;
}

message MotorRegisterWriteRequest = 25119 { // 0x621f
  required uint8 joint_id = 2;
  required uint8 register_id = 3;
  required float32 value = 4;
}

message MotorRegisterWriteResponse = 25120 { // 0x6220
}

message MotorStoreParametersRequest = 25121 { // 0x6221
  required uint8 joint_id = 2;
}

message MotorStoreParametersResponse = 25122 { // 0x6222
}

message MotorSetZeroRequest = 25123 { // 0x6223
  required uint8 joint_id = 2;
}

message MotorSetZeroResponse = 25124 { // 0x6224
}

message SetArmModeRequest = 25125 { // 0x6225
  required ArmMode mode = 2;
}

message SetArmModeResponse = 25126 { // 0x6226
}

message GetDeviceSettingsRequest = 25128 { // 0x6228
}

message GetDeviceSettingsResponse = 25129 { // 0x6229
  optional DeviceSettings settings = 3;
}

message SetDeviceSettingsRequest = 25130 { // 0x622a
  required DeviceSettings settings = 2;
}

message SetDeviceSettingsResponse = 25131 { // 0x622b
  // Present on DEVICE_SETTINGS_OK and contains the value actually persisted
  // and applied by the device. Hosts must cache this authoritative response,
  // not the requested value.
  optional DeviceSettings settings = 3;
}

// Unlike the legacy fire-and-forget notification, emergency stop is a
// reliable RPC with an application result. It intentionally does not require
// the control lease: any peer that can reach the endpoint may request safety.
message EmergencyStopRequest = 25347 { // 0x6303
}

message EmergencyStopResponse = 25348 { // 0x6304
}
