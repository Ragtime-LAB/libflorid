#include <gtest/gtest.h>
#include <florid/protocols/protocols.hpp>
#include <cstring>

using namespace florid::protocol;

class ParserTest : public ::testing::Test
{
protected:
    TcpParser<512> parser;
};

TEST_F(ParserTest, SinglePacket)
{
    JointCmdPacket pkt{};
    pkt.header.magic_word = 0xA5;
    pkt.header.type = PacketType::JointCommand;
    pkt.header.length = sizeof(JointCmdPacket);

    int called = 0;
    auto handler = [&](const auto&) { ++called; };

    auto result = parser.feed(reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt), handler);
    EXPECT_EQ(result.packets_processed, 1u);
    EXPECT_FALSE(result.has_remaining);
    EXPECT_FALSE(result.dispatch_failed);
    EXPECT_EQ(called, 1);
}

TEST_F(ParserTest, MultiplePacketsInOneFeed)
{
    JointCmdPacket pkt1{};
    pkt1.header.magic_word = 0xA5;
    pkt1.header.type = PacketType::JointCommand;
    pkt1.header.length = sizeof(JointCmdPacket);

    SessionStatusPacket pkt2{};
    pkt2.header.magic_word = 0xA5;
    pkt2.header.type = PacketType::SessionStatus;
    pkt2.header.length = sizeof(SessionStatusPacket);

    uint8_t buf[sizeof(JointCmdPacket) + sizeof(SessionStatusPacket)];
    std::memcpy(buf, &pkt1, sizeof(pkt1));
    std::memcpy(buf + sizeof(pkt1), &pkt2, sizeof(pkt2));

    int called = 0;
    auto handler = [&](const auto&) { ++called; };

    auto result = parser.feed(buf, sizeof(buf), handler);
    EXPECT_EQ(result.packets_processed, 2u);
    EXPECT_FALSE(result.has_remaining);
    EXPECT_FALSE(result.dispatch_failed);
    EXPECT_EQ(called, 2);
}

TEST_F(ParserTest, ArmStatusPacket)
{
    ArmStatusPacket pkt{};
    pkt.header.magic_word = 0xA5;
    pkt.header.type = PacketType::ArmState;
    pkt.header.length = sizeof(ArmStatusPacket);
    pkt.mode = ArmMode::RUNNING;
    pkt.state.tau[5] = 2.5;

    bool called = false;
    auto handler = [&](const auto& p)
    {
        if constexpr (std::is_same_v<std::decay_t<decltype(p)>, ArmStatusPacket>)
        {
            called = true;
            EXPECT_EQ(p.mode, ArmMode::RUNNING);
            EXPECT_FLOAT_EQ(p.state.tau[5], 2.5f);
        }
    };

    auto result = parser.feed(reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt), handler);
    EXPECT_EQ(result.packets_processed, 1u);
    EXPECT_FALSE(result.has_remaining);
    EXPECT_FALSE(result.dispatch_failed);
    EXPECT_TRUE(called);
}

TEST_F(ParserTest, SplitPacketTwoFeeds)
{
    JointCmdPacket pkt{};
    pkt.header.magic_word = 0xA5;
    pkt.header.type = PacketType::JointCommand;
    pkt.header.length = sizeof(JointCmdPacket);

    int called = 0;
    auto handler = [&](const auto&) { ++called; };

    const size_t first_half = sizeof(pkt) / 2;
    auto result1 = parser.feed(reinterpret_cast<uint8_t*>(&pkt), first_half, handler);
    EXPECT_EQ(result1.packets_processed, 0u);
    EXPECT_TRUE(result1.has_remaining);
    EXPECT_EQ(called, 0);

    auto result2 = parser.feed(reinterpret_cast<uint8_t*>(&pkt) + first_half, sizeof(pkt) - first_half, handler);
    EXPECT_EQ(result2.packets_processed, 1u);
    EXPECT_FALSE(result2.has_remaining);
    EXPECT_EQ(called, 1);
}

TEST_F(ParserTest, SplitHeaderThenPayload)
{
    JointCmdPacket pkt{};
    pkt.header.magic_word = 0xA5;
    pkt.header.type = PacketType::JointCommand;
    pkt.header.length = sizeof(JointCmdPacket);

    int called = 0;
    auto handler = [&](const auto&) { ++called; };

    auto result1 = parser.feed(reinterpret_cast<uint8_t*>(&pkt), 4, handler);
    EXPECT_EQ(result1.packets_processed, 0u);
    EXPECT_TRUE(result1.has_remaining);

    auto result2 = parser.feed(reinterpret_cast<uint8_t*>(&pkt) + 4, sizeof(pkt) - 4, handler);
    EXPECT_EQ(result2.packets_processed, 1u);
    EXPECT_FALSE(result2.has_remaining);
    EXPECT_EQ(called, 1);
}

TEST_F(ParserTest, MagicWordResync)
{
    JointCmdPacket pkt{};
    pkt.header.magic_word = 0xA5;
    pkt.header.type = PacketType::JointCommand;
    pkt.header.length = sizeof(JointCmdPacket);

    uint8_t buf[sizeof(JointCmdPacket) + 2];
    std::memset(buf, 0, sizeof(buf));
    buf[0] = 0x00;
    buf[1] = 0x11;
    std::memcpy(buf + 2, &pkt, sizeof(pkt));

    int called = 0;
    auto handler = [&](const auto&) { ++called; };

    auto result = parser.feed(buf, sizeof(buf), handler);
    EXPECT_EQ(result.packets_processed, 1u);
    EXPECT_FALSE(result.has_remaining);
    EXPECT_EQ(called, 1);
}

TEST_F(ParserTest, InvalidLengthSkippedThenValidPacket)
{
    uint8_t bad[8];
    std::memset(bad, 0, sizeof(bad));
    bad[0] = 0xA5;
    bad[1] = static_cast<uint8_t>(PacketType::JointCommand);
    bad[2] = 4;
    bad[3] = 0;

    JointCmdPacket good{};
    good.header.magic_word = 0xA5;
    good.header.type = PacketType::JointCommand;
    good.header.length = sizeof(JointCmdPacket);

    uint8_t buf[sizeof(bad) + sizeof(good)];
    std::memcpy(buf, bad, sizeof(bad));
    std::memcpy(buf + sizeof(bad), &good, sizeof(good));

    int called = 0;
    auto handler = [&](const auto&) { ++called; };

    auto result = parser.feed(buf, sizeof(buf), handler);
    EXPECT_EQ(result.packets_processed, 1u);
    EXPECT_FALSE(result.has_remaining);
    EXPECT_EQ(called, 1);
}

TEST_F(ParserTest, UnknownTypeConsumesPacketButDispatchFails)
{
    uint8_t buf[16];
    std::memset(buf, 0, sizeof(buf));
    buf[0] = 0xA5;
    buf[1] = 0xFF;
    *reinterpret_cast<uint16_t*>(buf + 2) = sizeof(buf);

    int called = 0;
    auto handler = [&](const auto&) { ++called; };

    auto result = parser.feed(buf, sizeof(buf), handler);
    EXPECT_EQ(result.packets_processed, 1u);
    EXPECT_TRUE(result.dispatch_failed);
    EXPECT_FALSE(result.has_remaining);
    EXPECT_EQ(called, 0);
}

TEST_F(ParserTest, BufferOverflowResetsParser)
{
    uint8_t large_buf[600];
    std::memset(large_buf, 0, sizeof(large_buf));

    int called = 0;
    auto handler = [&](const auto&) { ++called; };

    auto result = parser.feed(large_buf, sizeof(large_buf), handler);
    EXPECT_EQ(result.packets_processed, 0u);
    EXPECT_FALSE(result.has_remaining);
    EXPECT_EQ(called, 0);
}

TEST_F(ParserTest, PartialHeaderRemains)
{
    uint8_t buf[4] = {0xA5, 0x11, 0x00, 0x00};
    int called = 0;
    auto handler = [&](const auto&) { ++called; };

    auto result = parser.feed(buf, sizeof(buf), handler);
    EXPECT_EQ(result.packets_processed, 0u);
    EXPECT_TRUE(result.has_remaining);
    EXPECT_EQ(called, 0);
}

TEST_F(ParserTest, CompletePartialHeaderInSecondFeed)
{
    JointCmdPacket pkt{};
    pkt.header.magic_word = 0xA5;
    pkt.header.type = PacketType::JointCommand;
    pkt.header.length = sizeof(JointCmdPacket);

    int called = 0;
    auto handler = [&](const auto&) { ++called; };

    auto result1 = parser.feed(reinterpret_cast<uint8_t*>(&pkt), 4, handler);
    EXPECT_EQ(result1.packets_processed, 0u);
    EXPECT_TRUE(result1.has_remaining);

    auto result2 = parser.feed(reinterpret_cast<uint8_t*>(&pkt) + 4, sizeof(pkt) - 4, handler);
    EXPECT_EQ(result2.packets_processed, 1u);
    EXPECT_FALSE(result2.has_remaining);
    EXPECT_EQ(called, 1);
}

TEST_F(ParserTest, LengthExceedsBufferSize)
{
    uint8_t buf[16];
    std::memset(buf, 0, sizeof(buf));
    buf[0] = 0xA5;
    buf[1] = static_cast<uint8_t>(PacketType::JointCommand);
    *reinterpret_cast<uint16_t*>(buf + 2) = 600;

    int called = 0;
    auto handler = [&](const auto&) { ++called; };

    auto result = parser.feed(buf, sizeof(buf), handler);
    EXPECT_EQ(result.packets_processed, 0u);
    EXPECT_EQ(called, 0);
    EXPECT_TRUE(result.has_remaining);
}
