#include <gtest/gtest.h>
#include <florid/protocols/protocols.hpp>
#include <cstring>

using namespace florid::protocol;

TEST(Dispatcher, DispatchJointCmdPacket)
{
    JointCmdPacket pkt{};
    pkt.header.magic_word = 0xA5;
    pkt.header.type = PacketType::JointCommand;
    pkt.header.length = sizeof(JointCmdPacket);

    bool called = false;
    auto handler = [&](const auto& p)
    {
        called = true;
        EXPECT_EQ(reinterpret_cast<const void*>(&p), reinterpret_cast<const void*>(&pkt));
    };

    EXPECT_TRUE(Dispatcher<>::dispatch(reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt), handler));
    EXPECT_TRUE(called);
}

TEST(Dispatcher, DispatchCartesianPoseCmdPacket)
{
    CartesianPoseCmdPacket pkt{};
    pkt.header.magic_word = 0xA5;
    pkt.header.type = PacketType::CartesianPoseCommand;
    pkt.header.length = sizeof(CartesianPoseCmdPacket);
    pkt.timestamp = 1.618;
    pkt.pose.T[0] = 1.0;
    pkt.pose.T[15] = 1.0;

    bool called = false;
    auto handler = [&](const auto& p)
    {
        if constexpr (std::is_same_v<std::decay_t<decltype(p)>, CartesianPoseCmdPacket>)
        {
            called = true;
            EXPECT_DOUBLE_EQ(p.timestamp, 1.618);
            EXPECT_FLOAT_EQ(p.pose.T[0], 1.0f);
            EXPECT_FLOAT_EQ(p.pose.T[15], 1.0f);
        }
    };

    EXPECT_TRUE(Dispatcher<>::dispatch(reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt), handler));
    EXPECT_TRUE(called);
}

TEST(Dispatcher, DispatchCartesianVelocityCmdPacket)
{
    CartesianVelocityCmdPacket pkt{};
    pkt.header.magic_word = 0xA5;
    pkt.header.type = PacketType::CartesianVelocityCommand;
    pkt.header.length = sizeof(CartesianVelocityCmdPacket);
    pkt.v[0] = 0.1f;
    pkt.v[3] = 0.5f;

    bool called = false;
    auto handler = [&](const auto& p)
    {
        if constexpr (std::is_same_v<std::decay_t<decltype(p)>, CartesianVelocityCmdPacket>)
        {
            called = true;
            EXPECT_FLOAT_EQ(p.v[0], 0.1f);
            EXPECT_FLOAT_EQ(p.v[3], 0.5f);
        }
    };

    EXPECT_TRUE(Dispatcher<>::dispatch(reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt), handler));
    EXPECT_TRUE(called);
}

TEST(Dispatcher, DispatchSessionStatusPacket)
{
    SessionStatusPacket pkt{};
    pkt.header.magic_word = 0xA5;
    pkt.header.type = PacketType::SessionStatus;
    pkt.header.length = sizeof(SessionStatusPacket);
    pkt.status = SessionStatus::FAILURE;

    bool called = false;
    auto handler = [&](const auto& p)
    {
        if constexpr (std::is_same_v<std::decay_t<decltype(p)>, SessionStatusPacket>)
        {
            called = true;
            EXPECT_EQ(p.status, SessionStatus::FAILURE);
        }
    };

    EXPECT_TRUE(Dispatcher<>::dispatch(reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt), handler));
    EXPECT_TRUE(called);
}

TEST(Dispatcher, DispatchArmStatusPacket)
{
    ArmStatusPacket pkt{};
    pkt.header.magic_word = 0xA5;
    pkt.header.type = PacketType::ArmState;
    pkt.header.length = sizeof(ArmStatusPacket);
    pkt.mode = ArmMode::FAULT;
    pkt.state.q[3] = 1.23;

    bool called = false;
    auto handler = [&](const auto& p)
    {
        if constexpr (std::is_same_v<std::decay_t<decltype(p)>, ArmStatusPacket>)
        {
            called = true;
            EXPECT_EQ(p.mode, ArmMode::FAULT);
            EXPECT_FLOAT_EQ(p.state.q[3], 1.23f);
        }
    };

    EXPECT_TRUE(Dispatcher<>::dispatch(reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt), handler));
    EXPECT_TRUE(called);
}

TEST(Dispatcher, DispatchConfigCmdPacket)
{
    ConfigCmdPacket pkt{};
    pkt.header.magic_word = 0xA5;
    pkt.header.type = PacketType::ConfigCommand;
    pkt.header.length = sizeof(ConfigCmdPacket);
    pkt.config_type = ConfigType::SetJointImpedance;
    pkt.data[0] = 100.0f;
    pkt.data[1] = 200.0f;

    bool called = false;
    auto handler = [&](const auto& p)
    {
        if constexpr (std::is_same_v<std::decay_t<decltype(p)>, ConfigCmdPacket>)
        {
            called = true;
            EXPECT_EQ(p.config_type, ConfigType::SetJointImpedance);
            EXPECT_FLOAT_EQ(p.data[0], 100.0f);
            EXPECT_FLOAT_EQ(p.data[1], 200.0f);
        }
    };

    EXPECT_TRUE(Dispatcher<>::dispatch(reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt), handler));
    EXPECT_TRUE(called);
}

TEST(Dispatcher, WrongMagicWord)
{
    JointCmdPacket pkt{};
    pkt.header.magic_word = 0x00;
    pkt.header.type = PacketType::JointCommand;
    pkt.header.length = sizeof(JointCmdPacket);

    auto handler = [](const auto&) {};
    EXPECT_FALSE(Dispatcher<>::dispatch(reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt), handler));
}

TEST(Dispatcher, WrongLengthLessThanPacketSize)
{
    JointCmdPacket pkt{};
    pkt.header.magic_word = 0xA5;
    pkt.header.type = PacketType::JointCommand;
    pkt.header.length = sizeof(JointCmdPacket) - 1;

    auto handler = [](const auto&) {};
    EXPECT_FALSE(Dispatcher<>::dispatch(reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt), handler));
}

TEST(Dispatcher, WrongLengthParameterMismatch)
{
    JointCmdPacket pkt{};
    pkt.header.magic_word = 0xA5;
    pkt.header.type = PacketType::JointCommand;
    pkt.header.length = sizeof(JointCmdPacket);

    auto handler = [](const auto&) {};
    EXPECT_FALSE(Dispatcher<>::dispatch(reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt) + 1, handler));
    EXPECT_FALSE(Dispatcher<>::dispatch(reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt) - 1, handler));
}

TEST(Dispatcher, UnknownType)
{
    uint8_t buf[32];
    std::memset(buf, 0, sizeof(buf));
    buf[0] = 0xA5;
    buf[1] = 0xFF;
    *reinterpret_cast<uint16_t*>(buf + 2) = sizeof(buf);

    auto handler = [](const auto&) {};
    EXPECT_FALSE(Dispatcher<>::dispatch(buf, sizeof(buf), handler));
}

TEST(Dispatcher, DataTooShortForHeader)
{
    uint8_t buf[4] = {0xA5, 0x11, 0x00, 0x00};
    auto handler = [](const auto&) {};
    EXPECT_FALSE(Dispatcher<>::dispatch(buf, sizeof(buf), handler));
}

TEST(Dispatcher, SessionCfgPacketNotInDefaultRegistry)
{
    SessionCfgPacket pkt{};
    pkt.header.magic_word = 0xA5;
    pkt.header.type = PacketType::SessionConfig;
    pkt.header.length = sizeof(SessionCfgPacket);

    bool called = false;
    auto handler = [&](const auto&) { called = true; };

    EXPECT_FALSE(Dispatcher<>::dispatch(reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt), handler));
    EXPECT_FALSE(called);
}

TEST(Dispatcher, CustomRegistryCanDispatchSessionCfg)
{
    using CustomRegistry = PacketRegistry<SessionCfgPacket>;
    SessionCfgPacket pkt{};
    pkt.header.magic_word = 0xA5;
    pkt.header.type = PacketType::SessionConfig;
    pkt.header.length = sizeof(SessionCfgPacket);
    pkt.mode = SessionMode::Point;

    bool called = false;
    auto handler = [&](const auto& p)
    {
        called = true;
        EXPECT_EQ(p.mode, SessionMode::Point);
    };

    EXPECT_TRUE((Dispatcher<CustomRegistry>::dispatch(reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt), handler)));
    EXPECT_TRUE(called);
}
