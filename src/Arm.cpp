#include <florid/Arm.hpp>
#include <florid/detail/TcpClient.hpp>
#include <florid/detail/UdpClient.hpp>

namespace florid
{
    namespace
    {
        struct HostTransport
        {
            detail::TcpClient tcp;
            detail::UdpClient udp;

            HostTransport(const char* ip, uint16_t udp_port)
                : tcp(ip, 6041), udp(ip, udp_port) {}
        };
    }

    Arm::Arm(const char* ip, uint16_t udp_port, protocol::SessionMode mode)
        : m_max_frequency_hz(500.0)
        , m_next_tick_ms(0.0)
        , m_tick_initialized(false)
        , m_finish_flag(false)
    {
        auto* ht = new HostTransport(ip, udp_port);
        m_host_impl = ht;

        if (ht->tcp.is_connected())
        {
            ht->tcp.configure_session(mode, ht->udp.localPort());
            m_cfg_transport = &ht->tcp;
        }
        m_transport = &ht->udp;
        m_transport->set_receive_callback(&Arm::on_receive_thunk, this);
    }

    Arm::~Arm()
    {
        if (m_host_impl)
            delete static_cast<HostTransport*>(m_host_impl);
    }

    Arm::Arm(Transport& control_transport, Transport* config_transport)
        : m_transport(&control_transport)
        , m_cfg_transport(config_transport)
        , m_max_frequency_hz(500.0)
    {
        m_transport->set_receive_callback(&Arm::on_receive_thunk, this);
    }

void Arm::on_receive_thunk(void* context, const uint8_t* data, const size_t len)
{
    auto* self = static_cast<Arm*>(context);
    self->m_core.feed_incoming_data(data, len);
}

ArmState Arm::readOnce()
{
    return m_core.get_arm_state();
}

ArmState Arm::update()
{
    return updateThreadSafe();
}

ArmState Arm::updateThreadSafe()
{
    std::lock_guard<std::mutex> lock(m_io_mutex);
    m_transport->poll();
    return m_core.get_arm_state();
}

void Arm::beginControlSession()
{
    std::lock_guard<std::mutex> lock(m_io_mutex);
    if (!m_core.control_session_active())
        m_core.begin_control_session();
}

void Arm::endControlSession()
{
    std::lock_guard<std::mutex> lock(m_io_mutex);
    if (m_core.control_session_active())
        m_core.end_control_session();
}

bool Arm::isControlSessionActive()
{
    std::lock_guard<std::mutex> lock(m_io_mutex);
    return m_core.control_session_active();
}

uint64_t Arm::lastCommandTimestampUs()
{
    std::lock_guard<std::mutex> lock(m_io_mutex);
    return m_core.last_command_timestamp_us();
}

uint32_t Arm::lastCommandSeq()
{
    std::lock_guard<std::mutex> lock(m_io_mutex);
    return m_core.last_command_seq();
}

void Arm::setMaxFrequencyHz(const double hz)
{
    m_max_frequency_hz = (hz > 0.0 && hz <= 1000.0) ? hz : 0.0;
    m_next_tick_ms     = 0.0;
    m_tick_initialized = false;
}

// ── 配置发送辅助 ────────────────────────────────────────────────

namespace
{
    bool send_config(Transport* transport,
                     protocol::ConfigType type,
                     const float* data,
                     unsigned count)
    {
        if (!transport) return false;

        protocol::ConfigCmdPacket pkt{};
        pkt.config_type = type;
        pkt.header.length = static_cast<uint16_t>(sizeof(pkt));

        for (unsigned i = 0; i < count && i < 16; ++i)
            pkt.data[i] = data[i];

        return transport->send(reinterpret_cast<const uint8_t*>(&pkt), sizeof(pkt));
    }
}

void Arm::setCollisionBehavior(const float (&lower_torques)[6],
                                const float (&upper_torques)[6])
{
    float data[16]{};
    for (int i = 0; i < 6; ++i) data[i]     = lower_torques[i];
    for (int i = 0; i < 6; ++i) data[6 + i] = upper_torques[i];
    send_config(m_cfg_transport, protocol::ConfigType::SetCollisionBehavior, data, 12);
}

void Arm::setCartesianImpedance(const float (&K_x)[6])
{
    send_config(m_cfg_transport, protocol::ConfigType::SetCartesianImpedance, K_x, 6);
}

void Arm::setK(const float (&EE_T_K)[16])
{
    send_config(m_cfg_transport, protocol::ConfigType::SetK, EE_T_K, 16);
}

void Arm::setEE(const float (&NE_T_EE)[16])
{
    send_config(m_cfg_transport, protocol::ConfigType::SetEE, NE_T_EE, 16);
}

void Arm::setLoad(float mass,
                  const float (&F_x_Cload)[3],
                  const float (&load_inertia)[9])
{
    float data[16]{};
    data[0] = mass;
    for (int i = 0; i < 3; ++i)  data[1 + i]  = F_x_Cload[i];
    for (int i = 0; i < 9; ++i)  data[4 + i]  = load_inertia[i];
    send_config(m_cfg_transport, protocol::ConfigType::SetLoad, data, 13);
}

void Arm::automaticErrorRecovery()
{
    send_config(m_cfg_transport, protocol::ConfigType::AutomaticErrorRecovery, nullptr, 0);
}

void Arm::stop()
{
    send_config(m_cfg_transport, protocol::ConfigType::Stop, nullptr, 0);
}

void Arm::setWatchdogTimeout(Duration timeout)
{
    float data[1]{};
    data[0] = static_cast<float>(timeout.toMSec());
    send_config(m_cfg_transport, protocol::ConfigType::SetWatchdogTimeout, data, 1);
}

void Arm::switchControlMode(protocol::ControlMode mode)
{
    float data[1]{};
    data[0] = static_cast<float>(static_cast<uint8_t>(mode));
    send_config(m_cfg_transport, protocol::ConfigType::SwitchControlMode, data, 1);
}

ActiveControl<Torques> Arm::startTorqueControl(double rate_hz,
                                                double command_timeout_ms,
                                                bool auto_start)
{
    return ActiveControl<Torques>(*this, rate_hz, command_timeout_ms, auto_start);
}

ActiveControl<JointPositions> Arm::startJointPositionControl()
{
    return ActiveControl<JointPositions>(*this);
}

ActiveControl<JointVelocities> Arm::startJointVelocityControl()
{
    return ActiveControl<JointVelocities>(*this);
}

ActiveControl<CartesianPose> Arm::startCartesianPoseControl()
{
    return ActiveControl<CartesianPose>(*this);
}

ActiveControl<CartesianVelocities> Arm::startCartesianVelocityControl()
{
    return ActiveControl<CartesianVelocities>(*this);
}

} // namespace florid
