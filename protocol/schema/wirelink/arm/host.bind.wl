profile version 1;

// Compose with services.bind.wl; this file only describes the local endpoint.
endpoint {
  envelope = cobs_stream;
  rpc_role = client;
}

// Retain only this side's incoming streams.
latest ArmStatus { delivery = unreliable; }
latest ArmDiagnostics { delivery = unreliable; }
latest MotorFeedback { delivery = unreliable; }

// Sending contributes to payload bounds without allocating receive mailboxes.
send JointMitCommand { delivery = unreliable; }
send GripperMitCommand { delivery = unreliable; }
send JointPositionVelocityCommand { delivery = unreliable; }
send JointVelocityCommand { delivery = unreliable; }
send JointPvtCommand { delivery = unreliable; }
send CartesianPoseCommand { delivery = unreliable; }
send CartesianVelocityCommand { delivery = unreliable; }
send GripperPositionVelocityCommand { delivery = unreliable; }
send GripperVelocityCommand { delivery = unreliable; }
send GripperPvtCommand { delivery = unreliable; }
