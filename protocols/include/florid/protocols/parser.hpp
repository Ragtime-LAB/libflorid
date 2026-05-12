#ifndef FLORID_PROTOCOLS_PARSER_HPP
#define FLORID_PROTOCOLS_PARSER_HPP

#include <cstring>
#include "dispatcher.hpp"

namespace florid::protocol
{
    template <size_t BufferSize, typename Registry = AllProtocolRegistry>
    class TcpParser
    {
    public:
        TcpParser()
        {
            std::memset(m_buffer, 0, BufferSize);
        }

        struct FeedResult
        {
            size_t packets_processed; ///> 本次成功解析的包数
            bool has_remaining; ///> 缓冲区是否还有未解析数据
            bool dispatch_failed; ///> 是否有 dispatch 失败
        };

        template <typename Handler>
        FeedResult feed(const uint8_t* incoming_data, const size_t len, Handler&& handler)
        {
            if (len > BufferSize || m_write_idx > BufferSize - len)
            {
                shift_to_front();
            }
            if (len > BufferSize || m_write_idx > BufferSize - len)
            {
                reset();
                return {0, false, false};
            }

            std::memcpy(m_buffer + m_write_idx, incoming_data, len);
            m_write_idx += len;

            FeedResult result{0, false, false};

            while (true)
            {
                const size_t available = m_write_idx - m_read_idx;

                // 包头长度
                if (available < sizeof(PacketHeader))
                {
                    if (available > 0)
                    {
                        result.has_remaining = true;
                    }
                    break;
                }

                // 错位检查
                const auto* header = reinterpret_cast<const PacketHeader*>(m_buffer + m_read_idx);
                if (header->magic_word != 0xA5)
                {
                    m_read_idx++;
                    continue;
                }

                // 包长检查(必须小于缓冲区大小)
                if (header->length > BufferSize || header->length < sizeof(PacketHeader))
                {
                    m_read_idx++;
                    continue;
                }

                // 包完整性检查
                if (available < header->length)
                {
                    // 包不完整，等待下次解析
                    result.has_remaining = true;
                    break;
                }

                // 找到完整包：根据对齐情况选择 fast / slow path
                bool res_dist;
                if (m_read_idx % 8 == 0)
                {
                    // Fast path: 已 8 字节对齐，直接 dispatch（零拷贝）
                    res_dist = Dispatcher<Registry>::dispatch(m_buffer + m_read_idx, header->length,
                                                              std::forward<Handler>(handler));
                }
                else
                {
                    // Slow path: resync 导致未对齐，先拷贝到对齐栈缓冲区再 dispatch
                    // 当前最大包长 160 bytes，512 足够覆盖未来扩展
                    alignas(8) uint8_t temp[512];
                    std::memcpy(temp, m_buffer + m_read_idx, header->length);
                    res_dist = Dispatcher<Registry>::dispatch(temp, header->length,
                                                              std::forward<Handler>(handler));
                }

                if (!res_dist)
                {
                    result.dispatch_failed = true;
                }

                // 缓冲区消费
                m_read_idx += header->length;
                ++result.packets_processed;
            }

            // 清理，避免下次溢出
            shift_to_front();
            return result;
        }

        void reset()
        {
            m_write_idx = 0;
            m_read_idx = 0;
        }

    private:
        void shift_to_front()
        {
            if (m_read_idx > 0)
            {
                const size_t leftover = m_write_idx - m_read_idx;
                if (leftover > 0)
                {
                    std::memmove(m_buffer, m_buffer + m_read_idx, leftover);
                }
                m_write_idx = leftover;
                m_read_idx = 0;
            }
        }

        alignas(8) uint8_t m_buffer[BufferSize]{};
        std::size_t m_write_idx{};
        std::size_t m_read_idx{};
    };
}

#endif //FLORID_PROTOCOLS_PARSER_HPP
