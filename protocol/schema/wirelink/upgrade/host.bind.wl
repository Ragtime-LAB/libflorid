profile version 1;

// Product configuration enables the client side of these role-neutral RPC
// bindings. Bulk transfer messages remain direct dispatcher routes.
rpc GetBootStatus {
  request = GetBootStatusRequest;
  response = GetBootStatusResponse;
  request_operation_id = operation_id;
  response_operation_id = operation_id;
  response_status = status;
  request_delivery = reliable;
  response_delivery = reliable;
}

rpc StartUpgrade {
  request = StartUpgradeRequest;
  response = StartUpgradeResponse;
  request_operation_id = operation_id;
  response_operation_id = operation_id;
  response_status = status;
  request_delivery = reliable;
  response_delivery = reliable;
}

rpc Reboot {
  request = RebootRequest;
  response = RebootResponse;
  request_operation_id = operation_id;
  response_operation_id = operation_id;
  response_status = status;
  request_delivery = reliable;
  response_delivery = reliable;
}
