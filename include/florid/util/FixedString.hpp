#ifndef FLORID_FIXEDSTRING_HPP
#define FLORID_FIXEDSTRING_HPP

#include <cstddef>
#include <cstring>

// 禁用对 std::string 的依赖
#ifndef FLORID_NO_STD_STRING
#include <string>
#endif

namespace florid
{
    /**
     * @brief 零动态内存分配的定长字符串
     * @tparam Capacity 字符串的最大容量（不包含结尾的 '\0'）。
     * 比如 IP 地址最多 15 个字符 ("255.255.255.255")，Capacity 设为 16 即可。
     */
    template <size_t Capacity>
    class FixedString
    {
    public:
        // 默认构造：空字符串
        FixedString() noexcept
        {
            clear();
        }

        // 从 C 风格字符串构造 (截断超长部分，保证安全)
        FixedString(const char* str) noexcept
        {
            assign(str);
        }

#ifndef FLORID_NO_STD_STRING
        // 从 std::string 构造
        FixedString(const std::string& str) noexcept
        {
            assign(str.c_str(), str.length());
        }
#endif

        // 允许不同容量的 FixedString 互相构造/赋值
        template <size_t OtherCapacity>
        FixedString(const FixedString<OtherCapacity>& other) noexcept
        {
            assign(other.c_str(), other.size());
        }

        // 赋值运算符 (C 字符串)
        FixedString& operator=(const char* str) noexcept
        {
            assign(str);
            return *this;
        }

#ifndef FLORID_NO_STD_STRING
        // 赋值运算符 (std::string)
        FixedString& operator=(const std::string& str) noexcept
        {
            assign(str.c_str(), str.length());
            return *this;
        }
#endif

        // 获取裸指针（供 FlatBuffers、printf 或底层的 C API 使用）
        [[nodiscard]] const char* c_str() const noexcept { return m_data; }

        // 获取当前实际长度
        [[nodiscard]] size_t size() const noexcept { return m_length; }

        // 获取最大容量
        [[nodiscard]] static constexpr size_t capacity() noexcept { return Capacity; }

        [[nodiscard]] bool is_empty() const noexcept { return m_length == 0; }

        void clear() noexcept
        {
            m_length = 0;
            m_data[0] = '\0';
        }

#ifndef FLORID_NO_STD_STRING
        // 显示转回 std::string
        [[nodiscard]] std::string to_string() const
        {
            return std::string(m_data, m_length);
        }

        // 隐式类型转换 (允许直接将 FixedString 当作 std::string 传参)
        operator std::string() const
        {
            return to_string();
        }
#endif

        // 比较运算符
        bool operator==(const char* other) const noexcept
        {
            if (!other) return false;
            return std::strncmp(m_data, other, Capacity) == 0;
        }

        bool operator==(const FixedString& other) const noexcept
        {
            if (m_length != other.m_length) return false;
            return std::strncmp(m_data, other.m_data, Capacity) == 0;
        }

#ifndef FLORID_NO_STD_STRING
        bool operator==(const std::string& other) const noexcept
        {
            if (m_length != other.length()) return false;
            return std::strncmp(m_data, other.c_str(), Capacity) == 0;
        }
#endif

        bool operator!=(const char* other) const noexcept { return !(*this == other); }

    private:
        void assign(const char* str, const size_t len) noexcept
        {
            if (!str)
            {
                clear();
                return;
            }
            m_length = (len < Capacity) ? len : Capacity; // 截断防溢出
            std::memcpy(m_data, str, m_length);
            m_data[m_length] = '\0'; // 绝对保证以 null 结尾
        }

        void assign(const char* str) noexcept
        {
            if (!str)
            {
                clear();
            }
            else
            {
                assign(str, std::strlen(str));
            }
        }

        // 内部缓冲区：Capacity + 1 用来放 '\0'
        char m_data[Capacity + 1]{};
        size_t m_length{};
    };

    // 预定义一些常用的定长字符串类型
    using IpAddressStr = FixedString<16>; // "255.255.255.255" = 15 chars
    using VersionStr = FixedString<32>; // "v1.2.3-beta.4"
} // namespace florid

#endif //FLORID_FIXEDSTRING_HPP
