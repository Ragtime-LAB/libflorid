#ifndef FLORID_ARM_HPP
#define FLORID_ARM_HPP

#include "core/ArmCore.hpp"
#include "detail/tick.hpp"
#include "detail/Transport.hpp"
#include <stddef.h>

namespace florid {

/**
 * @brief 机械臂控制入口类（平台无关）。
 *
 * 通过注入不同的 Transport 实现，同一套 Arm API 可在 Host/RTOS/Arduino 等平台上复用。
 */
class Arm {
public:
    /**
     * @brief 通过具体 Transport 构造 Arm。
     * @param transport 平台相关的传输实现，如 detail::UdpClient、ArduinoUdpTransport。
     */
    explicit Arm(Transport& transport);

    Arm(const Arm&) = delete;
    Arm& operator=(const Arm&) = delete;
    Arm(Arm&&) = delete;
    Arm& operator=(Arm&&) = delete;
    ~Arm() = default;

    core::ArmStatus get_status();
    void set_control_frequency_hz(double hz);

    template <typename UserCommand>
    void send_command(const UserCommand& command)
    {
        auto raw_data = m_core.pack_command(command);
        m_transport->send(raw_data.data, raw_data.size);
    }

    /**
     * @brief 轮询接收数据。
     *
     * 在 Host 平台通常无需显式调用（内部线程已驱动异步接收）。
     * 在无线程的嵌入式平台（如 Arduino），请在主循环 loop() 中定期调用。
     */
    void update() { m_transport->poll(); }

    template <typename Callable>
    [[noreturn]] void control(Callable&& callable)
    {
        using CommandType = decltype(callable(m_core.get_current_status()));
        static_assert(
            core::is_control_command<CommandType>::value,
            "Control callback must return florid::core::JointCommand or florid::core::CartesianPose");

        while (true)
        {
            update(); // 驱动接收，确保在无线程平台也能获取状态

            if (m_control_period_ms > 0.0)
            {
                const double now = detail::get_tick_ms();
                if (!m_control_tick_initialized)
                {
                    m_next_control_tick_ms = now;
                    m_control_tick_initialized = true;
                }
                if (now < m_next_control_tick_ms)
                {
                    continue;
                }

                m_next_control_tick_ms += m_control_period_ms;
                if (m_next_control_tick_ms <= now)
                {
                    m_next_control_tick_ms = now + m_control_period_ms;
                }
            }

            auto status = m_core.get_current_status();
            auto cmd = callable(status);
            send_command(cmd);
        }
    }

private:
    static void on_receive_thunk(void* context, const uint8_t* data, size_t len);

    Transport* m_transport;
    core::ArmCore m_core;
    double m_control_period_ms;
    double m_next_control_tick_ms;
    bool m_control_tick_initialized;
};

} // namespace florid

#endif // FLORID_ARM_HPP
