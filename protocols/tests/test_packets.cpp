#include <gtest/gtest.h>
#include <florid/protocols/protocols.hpp>

using namespace florid::protocol;

TEST(Packets, PacketHeaderSize)
{
    EXPECT_EQ(sizeof(PacketHeader), 8u);
}

TEST(Packets, PacketHeaderDefaultMagic)
{
    PacketHeader h;
    EXPECT_EQ(h.magic_word, 0xA5);
}

TEST(Packets, JointCmdPacketTypeId)
{
    EXPECT_EQ(JointCmdPacket::TYPE_ID, PacketType::JointCommand);
}

TEST(Packets, JointCmdPacketSize)
{
    EXPECT_EQ(sizeof(JointCmdPacket), 144u); // header(8)+timestamp_us(8)+mode(1)+intent(1)+pad(2)+kp(24)+kd(24)+state(72)+trail(4)
}

TEST(Packets, CartesianPoseCmdPacketTypeId)
{
    EXPECT_EQ(CartesianPoseCmdPacket::TYPE_ID, PacketType::CartesianPoseCommand);
}

TEST(Packets, CartesianPoseCmdPacketSize)
{
    EXPECT_EQ(sizeof(CartesianPoseCmdPacket), 136u); // header(8)+timestamp_us(8)+mode(1)+intent(1)+pad(2)+kp(24)+kd(24)+pose(64)+trail(4)
}

TEST(Packets, CartesianVelocityCmdPacketTypeId)
{
    EXPECT_EQ(CartesianVelocityCmdPacket::TYPE_ID, PacketType::CartesianVelocityCommand);
}

TEST(Packets, CartesianVelocityCmdPacketSize)
{
    EXPECT_EQ(sizeof(CartesianVelocityCmdPacket), 96u); // header(8)+timestamp_us(8)+mode(1)+intent(1)+pad(2)+kp(24)+kd(24)+v[6](24)+trail(4)
}

TEST(Packets, SessionCfgPacketTypeId)
{
    EXPECT_EQ(SessionCfgPacket::TYPE_ID, PacketType::SessionConfig);
}

TEST(Packets, SessionCfgPacketSize)
{
    EXPECT_EQ(sizeof(SessionCfgPacket), 24u);
}

TEST(Packets, SessionStatusPacketTypeId)
{
    EXPECT_EQ(SessionStatusPacket::TYPE_ID, PacketType::SessionStatus);
}

TEST(Packets, ArmStatusPacketTypeId)
{
    EXPECT_EQ(ArmStatusPacket::TYPE_ID, PacketType::ArmState);
}

TEST(Packets, ArmStatusPacketSize)
{
    EXPECT_EQ(sizeof(ArmStatusPacket), 224u);
}

TEST(Packets, ConfigCmdPacketTypeId)
{
    EXPECT_EQ(ConfigCmdPacket::TYPE_ID, PacketType::ConfigCommand);
}

TEST(Packets, ConfigCmdPacketSize)
{
    EXPECT_EQ(sizeof(ConfigCmdPacket), 88u);
}

TEST(Packets, ArmModeValues)
{
    EXPECT_EQ(static_cast<uint8_t>(ArmMode::INIT), 0);
    EXPECT_EQ(static_cast<uint8_t>(ArmMode::IDLE), 1);
    EXPECT_EQ(static_cast<uint8_t>(ArmMode::RUNNING), 2);
    EXPECT_EQ(static_cast<uint8_t>(ArmMode::FAULT), 3);
    EXPECT_EQ(static_cast<uint8_t>(ArmMode::ESTOP), 4);
}

TEST(Packets, ArmStatusLayout)
{
    ArmStatusPacket as{};
    as.mode = ArmMode::RUNNING;
    as.state.q[0] = 1.0;
    as.state.dq[1] = 2.0;
    as.state.tau[2] = 3.0;
    as.errors = 0x42u;
    as.timestamp = 1.618;
    as.base_gravity[0] = 0.0f;
    as.base_gravity[1] = 1.0f;
    as.base_gravity[2] = -9.81f;
    EXPECT_EQ(as.mode, ArmMode::RUNNING);
    EXPECT_FLOAT_EQ(as.state.q[0], 1.0f);
    EXPECT_FLOAT_EQ(as.state.dq[1], 2.0f);
    EXPECT_FLOAT_EQ(as.state.tau[2], 3.0f);
    EXPECT_EQ(as.errors, 0x42u);
    EXPECT_DOUBLE_EQ(as.timestamp, 1.618);
    EXPECT_FLOAT_EQ(as.base_gravity[1], 1.0f);
    EXPECT_FLOAT_EQ(as.base_gravity[2], -9.81f);
}

TEST(Packets, JointStateLayout)
{
    JointState js{};
    js.q[0] = 1.0;
    js.q[5] = 6.0;
    js.dq[1] = 2.0;
    js.tau[2] = 3.0;
    EXPECT_FLOAT_EQ(js.q[0], 1.0f);
    EXPECT_FLOAT_EQ(js.q[5], 6.0f);
    EXPECT_FLOAT_EQ(js.dq[1], 2.0f);
    EXPECT_FLOAT_EQ(js.tau[2], 3.0f);
}

TEST(Packets, CartesianPoseLayout)
{
    CartesianPose pose{};
    pose.T[0] = 1.0;
    pose.T[5] = 1.0;
    pose.T[10] = 1.0;
    pose.T[15] = 1.0;
    pose.T[3] = 0.5;
    EXPECT_FLOAT_EQ(pose.T[0], 1.0f);
    EXPECT_FLOAT_EQ(pose.T[5], 1.0f);
    EXPECT_FLOAT_EQ(pose.T[10], 1.0f);
    EXPECT_FLOAT_EQ(pose.T[15], 1.0f);
    EXPECT_FLOAT_EQ(pose.T[3], 0.5f);
}

TEST(Packets, SessionModeValues)
{
    EXPECT_EQ(static_cast<uint8_t>(SessionMode::Joint), 0);
    EXPECT_EQ(static_cast<uint8_t>(SessionMode::Point), 1);
    EXPECT_EQ(static_cast<uint8_t>(SessionMode::Stop), 2);
}

TEST(Packets, SessionStatusValues)
{
    EXPECT_EQ(static_cast<uint8_t>(SessionStatus::SUCCESS), 0);
    EXPECT_EQ(static_cast<uint8_t>(SessionStatus::FAILURE), 1);
}

TEST(Packets, ControlModeValues)
{
    EXPECT_EQ(static_cast<uint8_t>(ControlMode::JointPosition), 0x01);
    EXPECT_EQ(static_cast<uint8_t>(ControlMode::JointVelocity), 0x02);
    EXPECT_EQ(static_cast<uint8_t>(ControlMode::Torque), 0x03);
    EXPECT_EQ(static_cast<uint8_t>(ControlMode::CartesianPose), 0x04);
    EXPECT_EQ(static_cast<uint8_t>(ControlMode::CartesianVelocity), 0x05);
}

TEST(Packets, ConfigTypeValues)
{
    EXPECT_EQ(static_cast<uint8_t>(ConfigType::Stop), 0x01);
    EXPECT_EQ(static_cast<uint8_t>(ConfigType::AutomaticErrorRecovery), 0x02);
    EXPECT_EQ(static_cast<uint8_t>(ConfigType::SetCollisionBehavior), 0x10);
    EXPECT_EQ(static_cast<uint8_t>(ConfigType::SetJointImpedance), 0x11);
    EXPECT_EQ(static_cast<uint8_t>(ConfigType::SetCartesianImpedance), 0x12);
    EXPECT_EQ(static_cast<uint8_t>(ConfigType::SetK), 0x13);
    EXPECT_EQ(static_cast<uint8_t>(ConfigType::SetEE), 0x14);
    EXPECT_EQ(static_cast<uint8_t>(ConfigType::SetLoad), 0x15);
    EXPECT_EQ(static_cast<uint8_t>(ConfigType::SetWatchdogTimeout), 0x16);
    EXPECT_EQ(static_cast<uint8_t>(ConfigType::SwitchControlMode), 0x20);
}

TEST(Packets, JointCmdHasControlMode)
{
    JointCmdPacket pkt{};
    pkt.control_mode = ControlMode::Torque;
    EXPECT_EQ(pkt.control_mode, ControlMode::Torque);
}

TEST(Packets, JointCmdDefaultIntent)
{
    JointCmdPacket pkt{};
    EXPECT_EQ(pkt.intent, CommandIntent::PointTarget);
}

TEST(Packets, JointCmdHasGains)
{
    JointCmdPacket pkt{};
    pkt.kp[0] = 100.0f;
    pkt.kd[3] = 5.0f;
    EXPECT_FLOAT_EQ(pkt.kp[0], 100.0f);
    EXPECT_FLOAT_EQ(pkt.kd[3], 5.0f);
    // default is zero
    EXPECT_FLOAT_EQ(pkt.kp[2], 0.0f);
}

TEST(Packets, CartesianPoseCmdHasControlMode)
{
    CartesianPoseCmdPacket pkt{};
    pkt.control_mode = ControlMode::CartesianPose;
    EXPECT_EQ(pkt.control_mode, ControlMode::CartesianPose);
}

TEST(Packets, CartesianPoseCmdHasJointGains)
{
    CartesianPoseCmdPacket pkt{};
    pkt.kp[1] = 42.0f;
    pkt.kd[4] = 3.5f;
    EXPECT_FLOAT_EQ(pkt.kp[1], 42.0f);
    EXPECT_FLOAT_EQ(pkt.kd[4], 3.5f);
}

TEST(Packets, CartesianVelocityCmdDefaultControlMode)
{
    CartesianVelocityCmdPacket pkt{};
    EXPECT_EQ(pkt.control_mode, ControlMode::CartesianVelocity);
}

TEST(Packets, CartesianVelocityCmdHasJointGains)
{
    CartesianVelocityCmdPacket pkt{};
    pkt.kp[2] = 12.0f;
    pkt.kd[5] = 0.8f;
    EXPECT_FLOAT_EQ(pkt.kp[2], 12.0f);
    EXPECT_FLOAT_EQ(pkt.kd[5], 0.8f);
}

TEST(Packets, SessionCfgHasWatchdogTimeout)
{
    SessionCfgPacket cfg{};
    EXPECT_EQ(cfg.watchdog_timeout_ms, 500u);
}

TEST(Packets, SessionCfgClientUdpPort)
{
    SessionCfgPacket cfg{};
    EXPECT_EQ(cfg.client_udp_port, 0u);  // default = no telemetry
}
