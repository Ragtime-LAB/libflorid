#ifndef FLORID_TRANSPORT_HPP
#define FLORID_TRANSPORT_HPP

#include <stddef.h>
#include <cstdint>

namespace florid
{
    /**
     * @brief 平台无关的传输层抽象接口。
     *
     * 设计目标：让 florid::Arm 的上层控制逻辑与底层网络实现完全解耦，
     * 从而在不同平台上可通过注入不同的实现来复用同一套 SDK 核心。
     *
     * 适用平台举例：
     * - Host (Win/Linux/Mac): AsioUdpTransport —— 内部线程 + io_context 驱动
     * - Arduino (ESP32/等):   用户基于 WiFiUDP / EthernetUDP 实现
     * - 裸机 / RTOS:          用户基于 lwIP socket 或串口实现
     */
    class Transport
    {
    public:
        using ReceiveFunctor = void (*)(void* context, const uint8_t* data, size_t size);

        virtual ~Transport() = default;

        /**
         * @brief 发送数据到远端。
         * @return true 表示成功发出（或成功入队）。
         */
        virtual bool send(const uint8_t* data, size_t size) = 0;

        /**
         * @brief 设置接收到数据时的回调。
         *
         * 回调由底层在收到数据后调用。具体调用上下文取决于实现：
         * - AsioUdpTransport：在内部 io_context 线程中调用。
         * - 轮询式实现：在 poll() 中被同步调用。
         */
        virtual void set_receive_callback(ReceiveFunctor callback, void* context) = 0;

        /**
         * @brief 轮询接收数据。
         *
         * 在 Host 平台（如 AsioUdpTransport），内部通常有独立线程驱动异步接收，
         * 此时 poll() 可以为空操作。
         *
         * 在无线程的嵌入式平台（如 Arduino），需要在主循环中定期调用此方法
         * 以驱动数据接收并触发回调。
         */
        virtual void poll() = 0;
    };
} // namespace florid

#endif // FLORID_TRANSPORT_HPP
