#include <gtest/gtest.h>
#include <florid/Arm.hpp>
#include <florid/protocols/protocols.hpp>

#include <chrono>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

namespace
{
    class MockTransport final : public florid::Transport
    {
    public:
        bool send(const uint8_t* data, size_t size) override
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_sent.emplace_back(data, data + size);
            return true;
        }

        void set_receive_callback(ReceiveFunctor callback, void* context) override
        {
            m_callback = callback;
            m_context = context;
        }

        void poll() override {}

        size_t sentCount() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_sent.size();
        }

        florid::protocol::JointCmdPacket lastJointCommand() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            florid::protocol::JointCmdPacket packet{};
            if (!m_sent.empty() && m_sent.back().size() == sizeof(packet))
                std::memcpy(&packet, m_sent.back().data(), sizeof(packet));
            return packet;
        }

    private:
        ReceiveFunctor m_callback{};
        void* m_context{};
        mutable std::mutex m_mutex;
        std::vector<std::vector<uint8_t>> m_sent;
    };

    void sleep_ms(int ms)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
}

TEST(ActiveTorqueControl, WaitsForFirstCommandThenStreamsUntilTimeout)
{
    MockTransport transport;
    florid::Arm arm(transport);

    auto control = arm.startTorqueControl(100.0, 30.0);
    sleep_ms(35);
    EXPECT_EQ(transport.sentCount(), 0u);

    florid::Torques cmd{};
    cmd.tau[0] = 1.25f;
    control.writeOnce(cmd);
    sleep_ms(35);

    const auto sent_after_command = transport.sentCount();
    EXPECT_GT(sent_after_command, 0u);
    auto packet = transport.lastJointCommand();
    EXPECT_EQ(packet.control_mode, florid::protocol::ControlMode::Torque);
    EXPECT_EQ(packet.intent, florid::protocol::CommandIntent::Stream);
    EXPECT_GT(packet.timestamp_us, 0u);
    EXPECT_GT(packet.header.seq_num, 0u);
    EXPECT_FLOAT_EQ(packet.state.tau[0], 1.25f);

    sleep_ms(55);
    const auto sent_after_timeout = transport.sentCount();
    sleep_ms(35);
    EXPECT_EQ(transport.sentCount(), sent_after_timeout);

    auto diag = control.diagnostics();
    EXPECT_GT(diag.sent_count, 0u);
    EXPECT_GT(diag.stale_command_count, 0u);
    EXPECT_EQ(diag.last_sdk_seq, diag.sent_count);

    control.stop();
    control.stop();
    const auto sent_after_stop = transport.sentCount();
    sleep_ms(20);
    EXPECT_EQ(transport.sentCount(), sent_after_stop);

    arm.writeOnce(cmd);
    auto point_packet = transport.lastJointCommand();
    EXPECT_EQ(point_packet.intent, florid::protocol::CommandIntent::PointTarget);
    EXPECT_EQ(point_packet.timestamp_us, 0u);
}
