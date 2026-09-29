profile version 1;

// Managed RPC v2 owns call/session IDs and transport status. Business results
// remain in the DTO alongside an actionable status snapshot on every reply.
rpc GetUnderwaterStatus {
  request = GetUnderwaterStatusRequest;
  response = GetUnderwaterStatusResponse;
}

rpc SetUnderwaterMode {
  request = SetUnderwaterModeRequest;
  response = SetUnderwaterModeResponse;
}

rpc GetUnderwaterSettings {
  request = GetUnderwaterSettingsRequest;
  response = GetUnderwaterSettingsResponse;
}

rpc SetUnderwaterSettings {
  request = SetUnderwaterSettingsRequest;
  response = SetUnderwaterSettingsResponse;
}

rpc GetUnderwaterDefaultSettings {
  request = GetUnderwaterDefaultSettingsRequest;
  response = GetUnderwaterDefaultSettingsResponse;
}

rpc Zeroing {
  request = ZeroingRequest;
  response = ZeroingResponse;
}
