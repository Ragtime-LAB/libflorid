#include "florid/detail/ArmImpl.hpp"
#include "florid/Exceptions.hpp"
#include "florid/detail/tick.hpp"

#include <thread>

namespace florid {

// ────────────────────────────────────────────────────────
//  ArmControl
// ────────────────────────────────────────────────────────

Duration ArmControl::firmwarePeriod() const {
    return Duration::fromUSec(m_impl ? m_impl->m_fw_dt_us : 2000);
}

Duration ArmControl::stateAge() const {
    // stub — to be implemented when latency estimator is ready
    return Duration::fromMSec(0);
}

Duration ArmControl::estimatedLatency() const {
    // stub — to be implemented when latency estimator is ready
    return Duration::fromMSec(0);
}

bool ArmControl::isReconnecting() const {
    return m_impl ? m_impl->m_reconnecting.load() : false;
}

void ArmControl::finishMotion() {
    if (m_impl) m_impl->m_stop_flag = true;
}

void ArmControl::stopControl() {
    if (m_impl) {
        m_impl->m_stop_flag = true;
        m_impl->m_running = false;
    }
}

// ────────────────────────────────────────────────────────
//  ArmImpl
// ────────────────────────────────────────────────────────

ArmImpl::ArmImpl(std::unique_ptr<Transport> s_transport)
    : m_transport(std::move(s_transport))
{
    m_arm_control.m_impl = this;

    // 1. Wire up protocol session → physical transport (send direction)
    m_session.on_send([this](const std::uint8_t* s_data, std::size_t s_size) {
        m_transport->send(s_data, s_size);
    });

    // 2. Wire up physical transport → ArmImpl (receive direction)
    m_transport->setReceiveCallback(s_onPhysData, this);

    // 3. Fetch device info via request/response
    s_fetchDeviceInfo();

    m_connected = true;
}

ArmImpl::~ArmImpl() {
    m_running = false;
}

void ArmImpl::s_onPhysData(void* s_context, const std::uint8_t* s_data, std::size_t s_size) {
    auto* s_self = static_cast<ArmImpl*>(s_context);
    s_self->s_feedBytes(s_data, s_size);
}

void ArmImpl::s_feedBytes(const std::uint8_t* s_data, std::size_t s_size) {
    // Feed into protocol RPL parser
    auto s_result = m_session.receive(s_data, s_size);
    if (!s_result) {
        // Protocol parsing error — ignore malformed packets, continue
        return;
    }

    // Check if a new ArmStatus was deserialized
    auto s_status = m_session.deserializer().get<fci::arm::ArmStatus>();
    if (s_status.seq != m_last_status_seq) {
        m_last_status_seq = s_status.seq;

        // Push to SPSC queue for the control thread
        if (!m_rx_queue.enqueue(s_status)) {
            // Queue full — drop oldest (or skip, depending on design)
            // For now, just skip enqueue; the control thread will pick up next one
            return;
        }

        m_data_ready.release();
    }
}

void ArmImpl::s_fetchDeviceInfo() {
    fci::arm::GetDeviceInfoRequestPacket s_req{};
    s_req.payload.dummy = 0;

    // request() allocates req_id internally via ack_mgr_.allocate()
    auto s_result = m_session.request(s_req, 100);
    if (!s_result) {
        // DeviceInfo fetch failed — use defaults
        m_device_info = {};
        m_fw_dt_us = 2000;
        return;
    }

    // Read the response from the deserializer
    auto s_response = m_session.deserializer().get<fci::arm::GetDeviceInfoResponsePacket>();
    m_device_info = s_response.payload.info;
    m_fw_dt_us = m_device_info.firmware_dt_us;
    if (m_fw_dt_us == 0) {
        m_fw_dt_us = 2000;
    }
}

ArmState ArmImpl::readOnce() {
    // Pop the latest state from the SPSC queue
    // If empty, block briefly
    fci::arm::ArmStatus s_raw;
    auto s_got = m_rx_queue.try_dequeue(s_raw);
    if (!s_got) {
        return ArmState{};
    }

    // Convert protocol ArmStatus → SDK ArmState
    ArmState s_state;
    s_state.m_time = detail::get_tick_ms();
    s_state.m_seq = s_raw.seq;
    s_state.m_source_timestamp_us = s_raw.timestamp_us;
    s_state.m_mode = static_cast<ArmMode>(static_cast<std::uint8_t>(s_raw.mode));
    s_state.m_errors = s_raw.errors;

    for (int s_i = 0; s_i < 6; ++s_i) {
        s_state.m_q[s_i] = s_raw.status.q[s_i];
        s_state.m_dq[s_i] = s_raw.status.dq[s_i];
        s_state.m_tau[s_i] = s_raw.status.tau[s_i];
    }

    for (int s_i = 0; s_i < 3; ++s_i) {
        s_state.m_base_gravity[s_i] = s_raw.base_gravity[s_i];
    }

    for (int s_i = 0; s_i < 16; ++s_i) {
        s_state.m_O_T_EE[s_i] = s_raw.O_T_EE[s_i];
    }

    for (int s_i = 0; s_i < 6; ++s_i) {
        s_state.m_F_ext[s_i] = s_raw.F_ext[s_i];
    }

    s_state.m_gripper_q = s_raw.gripper.q;
    s_state.m_gripper_dq = s_raw.gripper.dq;
    s_state.m_gripper_tau = s_raw.gripper.tau;

    return s_state;
}

} // namespace florid
