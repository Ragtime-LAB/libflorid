#ifndef FLORID_DURATION_HPP
#define FLORID_DURATION_HPP

#include <cstdint>

namespace florid
{
    class Duration
    {
    public:
        Duration() noexcept : m_ms(0) {}

        explicit Duration(uint64_t milliseconds) noexcept : m_ms(milliseconds) {}

        double toSec() const noexcept { return static_cast<double>(m_ms) / 1000.0; }
        uint64_t toMSec() const noexcept { return m_ms; }

        Duration operator+(Duration other) const noexcept { return Duration(m_ms + other.m_ms); }
        Duration operator-(Duration other) const noexcept { return Duration(m_ms - other.m_ms); }
        Duration operator*(uint64_t factor) const noexcept { return Duration(m_ms * factor); }
        Duration operator/(uint64_t divisor) const noexcept { return Duration(m_ms / divisor); }

        Duration& operator+=(Duration other) noexcept { m_ms += other.m_ms; return *this; }
        Duration& operator-=(Duration other) noexcept { m_ms -= other.m_ms; return *this; }
        Duration& operator*=(uint64_t factor) noexcept { m_ms *= factor; return *this; }
        Duration& operator/=(uint64_t divisor) noexcept { m_ms /= divisor; return *this; }

        bool operator==(Duration other) const noexcept { return m_ms == other.m_ms; }
        bool operator!=(Duration other) const noexcept { return m_ms != other.m_ms; }
        bool operator< (Duration other) const noexcept { return m_ms <  other.m_ms; }
        bool operator<=(Duration other) const noexcept { return m_ms <= other.m_ms; }
        bool operator> (Duration other) const noexcept { return m_ms >  other.m_ms; }
        bool operator>=(Duration other) const noexcept { return m_ms >= other.m_ms; }

        static Duration fromSec(double seconds) noexcept
        {
            return Duration(static_cast<uint64_t>(seconds * 1000.0));
        }

    private:
        uint64_t m_ms;
    };

} // namespace florid

#endif // FLORID_DURATION_HPP
