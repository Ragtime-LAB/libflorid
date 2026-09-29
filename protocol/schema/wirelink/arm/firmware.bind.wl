profile version 1;

// Compose with services.bind.wl; this file only describes the local endpoint.
endpoint {
  envelope = cobs_stream;
  rpc_role = server;
}

// Retain only this side's incoming streams.
latest JointMitCommand { delivery = unreliable; }
latest GripperMitCommand { delivery = unreliable; }
latest JointPositionVelocityCommand { delivery = unreliable; }
latest JointVelocityCommand { delivery = unreliable; }
latest JointPvtCommand { delivery = unreliable; }
latest CartesianPoseCommand { delivery = unreliable; }
latest CartesianVelocityCommand { delivery = unreliable; }
latest GripperPositionVelocityCommand { delivery = unreliable; }
latest GripperVelocityCommand { delivery = unreliable; }
latest GripperPvtCommand { delivery = unreliable; }

// Sending contributes to payload bounds without allocating receive mailboxes.
send ArmStatus { delivery = unreliable; }
send ArmDiagnostics { delivery = unreliable; }
send MotorFeedback { delivery = unreliable; }
