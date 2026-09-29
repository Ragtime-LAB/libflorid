profile version 1;

// Shared managed RPC contract. Reliability defaults to reliable.
rpc Home {
  request = HomeRequest;
  response = HomeResponse;
}

rpc GetDeviceInfo {
  request = GetDeviceInfoRequest;
  response = GetDeviceInfoResponse;
}

rpc SetDeviceInfo {
  request = SetDeviceInfoRequest;
  response = SetDeviceInfoResponse;
}

rpc SetArmControlMode {
  request = SetArmControlModeRequest;
  response = SetArmControlModeResponse;
}

rpc SetGripperControlMode {
  request = SetGripperControlModeRequest;
  response = SetGripperControlModeResponse;
}

rpc SetArmMode {
  request = SetArmModeRequest;
  response = SetArmModeResponse;
}

rpc MotorRegisterRead {
  request = MotorRegisterReadRequest;
  response = MotorRegisterReadResponse;
}

rpc MotorRegisterWrite {
  request = MotorRegisterWriteRequest;
  response = MotorRegisterWriteResponse;
}

rpc MotorStoreParameters {
  request = MotorStoreParametersRequest;
  response = MotorStoreParametersResponse;
}

rpc MotorSetZero {
  request = MotorSetZeroRequest;
  response = MotorSetZeroResponse;
}

rpc GetDeviceSettings {
  request = GetDeviceSettingsRequest;
  response = GetDeviceSettingsResponse;
}

rpc SetDeviceSettings {
  request = SetDeviceSettingsRequest;
  response = SetDeviceSettingsResponse;
}

rpc SetZero {
  request = SetZeroRequest;
  response = SetZeroResponse;
}

rpc ClearError {
  request = ClearErrorRequest;
  response = ClearErrorResponse;
}

rpc ClearFaults {
  request = ClearFaultsRequest;
  response = ClearFaultsResponse;
}

rpc GetMotorFeedback {
  request = GetMotorFeedbackRequest;
  response = GetMotorFeedbackResponse;
}

rpc EmergencyStop {
  request = EmergencyStopRequest;
  response = EmergencyStopResponse;
}

rpc AcquireControlLease {
  request = AcquireControlLeaseRequest;
  response = AcquireControlLeaseResponse;
}

rpc ReleaseControlLease {
  request = ReleaseControlLeaseRequest;
  response = ReleaseControlLeaseResponse;
}
