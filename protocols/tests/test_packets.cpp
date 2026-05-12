#include <gtest/gtest.h>
#include <florid/protocols/protocols.hpp>

using namespace florid::protocol;

TEST(Packets, PacketHeaderSize)
{
    EXPECT_EQ(sizeof(PacketHeader), 8u); // 1 + 1 + 2 + 4 = 8
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
    EXPECT_EQ(sizeof(JointCmdPacket), 88u); // header(8) + timestamp(8) + state(72)
}

TEST(Packets, CartesianPoseCmdPacketTypeId)
{
    EXPECT_EQ(CartesianPoseCmdPacket::TYPE_ID, PacketType::CartesianPoseCommand);
}

TEST(Packets, CartesianPoseCmdPacketSize)
{
    EXPECT_EQ(sizeof(CartesianPoseCmdPacket), 80u); // header(8) + timestamp(8) + pose(64)
}

TEST(Packets, SessionCfgPacketTypeId)
{
    EXPECT_EQ(SessionCfgPacket::TYPE_ID, PacketType::SessionConfig);
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
    EXPECT_EQ(sizeof(ArmStatusPacket), 88u); // header(8) + mode(1) + _pad(7) + state(72)
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
    EXPECT_EQ(as.mode, ArmMode::RUNNING);
    EXPECT_FLOAT_EQ(as.state.q[0], 1.0f);
    EXPECT_FLOAT_EQ(as.state.dq[1], 2.0f);
    EXPECT_FLOAT_EQ(as.state.tau[2], 3.0f);
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
