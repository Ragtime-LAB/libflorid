profile version 1;

// Firmware enables the server side of the role-neutral RPC bindings below.
latest DualJointMitCommand {
  delivery = unreliable;
}

latest DualGripperMitCommand {
  delivery = unreliable;
}

rpc GetDualDeviceInfo {
  request = GetDualDeviceInfoRequest;
  response = GetDualDeviceInfoResponse;
  request_operation_id = operation_id;
  response_operation_id = operation_id;
  response_status = status;
  request_delivery = reliable;
  response_delivery = reliable;
}

rpc SetDualDeviceInfo {
  request = SetDualDeviceInfoRequest;
  response = SetDualDeviceInfoResponse;
  request_operation_id = operation_id;
  response_operation_id = operation_id;
  response_status = status;
  request_delivery = reliable;
  response_delivery = reliable;
}

rpc SetDualArmControlMode {
  request = SetDualArmControlModeRequest;
  response = SetDualArmControlModeResponse;
  request_operation_id = operation_id;
  response_operation_id = operation_id;
  response_status = status;
  request_delivery = reliable;
  response_delivery = reliable;
}

rpc SetDualGripperControlMode {
  request = SetDualGripperControlModeRequest;
  response = SetDualGripperControlModeResponse;
  request_operation_id = operation_id;
  response_operation_id = operation_id;
  response_status = status;
  request_delivery = reliable;
  response_delivery = reliable;
}

rpc SetDualArmMode {
  request = SetDualArmModeRequest;
  response = SetDualArmModeResponse;
  request_operation_id = operation_id;
  response_operation_id = operation_id;
  response_status = status;
  request_delivery = reliable;
  response_delivery = reliable;
}

rpc ClearDualFaults {
  request = ClearDualFaultsRequest;
  response = ClearDualFaultsResponse;
  request_operation_id = operation_id;
  response_operation_id = operation_id;
  response_status = status;
  request_delivery = reliable;
  response_delivery = reliable;
}

rpc DualEmergencyStop {
  request = DualEmergencyStopRequest;
  response = DualEmergencyStopResponse;
  request_operation_id = operation_id;
  response_operation_id = operation_id;
  response_status = status;
  request_delivery = reliable;
  response_delivery = reliable;
}

rpc AcquireDualControlLease {
  request = AcquireDualControlLeaseRequest;
  response = AcquireDualControlLeaseResponse;
  request_operation_id = operation_id;
  response_operation_id = operation_id;
  response_status = status;
  request_delivery = reliable;
  response_delivery = reliable;
}

rpc ReleaseDualControlLease {
  request = ReleaseDualControlLeaseRequest;
  response = ReleaseDualControlLeaseResponse;
  request_operation_id = operation_id;
  response_operation_id = operation_id;
  response_status = status;
  request_delivery = reliable;
  response_delivery = reliable;
}
