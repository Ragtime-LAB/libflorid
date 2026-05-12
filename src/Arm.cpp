#include <florid/Arm.hpp>

namespace florid {

Arm::Arm(Transport& transport)
    : m_transport(&transport),
      m_control_period_ms(0.0),
      m_next_control_tick_ms(0.0),
      m_control_tick_initialized(false)
{
    m_transport->set_receive_callback(&Arm::on_receive_thunk, this);
}

void Arm::on_receive_thunk(void* context, const uint8_t* data, const size_t len)
{
    auto* self = static_cast<Arm*>(context);
    self->m_core.feed_incoming_data(data, len);
}

core::ArmStatus Arm::get_status()
{
    return m_core.get_current_status();
}

void Arm::set_control_frequency_hz(const double hz)
{
    if (hz <= 0.0)
    {
        m_control_period_ms = 0.0;
        m_next_control_tick_ms = 0.0;
        m_control_tick_initialized = false;
        return;
    }

    m_control_period_ms = 1000.0 / hz;
    m_next_control_tick_ms = 0.0;
    m_control_tick_initialized = false;
}

} // namespace florid
