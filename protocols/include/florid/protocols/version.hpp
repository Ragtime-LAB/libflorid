#ifndef FLORID_PROTOCOLS_VERSION_HPP
#define FLORID_PROTOCOLS_VERSION_HPP

#include <cstdint>

namespace florid::protocol
{
    /**
     * @brief 协议版本号，将 x.y.z 打包进一个 uint16_t。
     *
     * 位域分配：
     *   - major (x) : 4 bits  [15:12]  范围 0..15
     *   - minor (y) : 6 bits  [11:6]   范围 0..63
     *   - patch (z) : 6 bits  [5:0]    范围 0..63
     *
     * 兼容性规则：仅比较 major 版本号；minor 与 patch 的差异视为兼容。
     */
    struct Version
    {
        uint16_t raw = 0;

        constexpr Version() = default;

        constexpr Version(uint8_t major, uint8_t minor, uint8_t patch) noexcept
            : raw(encode(major, minor, patch))
        {
        }

        explicit constexpr Version(uint16_t raw_value) noexcept
            : raw(raw_value)
        {
        }

        /// @brief 将 x.y.z 编码为 uint16_t。
        static constexpr uint16_t encode(uint8_t major, uint8_t minor, uint8_t patch) noexcept
        {
            return (static_cast<uint16_t>(major & 0x0Fu) << 12) |
                   (static_cast<uint16_t>(minor & 0x3Fu) << 6) |
                   (static_cast<uint16_t>(patch & 0x3Fu));
        }

        constexpr uint8_t major() const noexcept { return static_cast<uint8_t>((raw >> 12) & 0x0Fu); }
        constexpr uint8_t minor() const noexcept { return static_cast<uint8_t>((raw >> 6) & 0x3Fu); }
        constexpr uint8_t patch() const noexcept { return static_cast<uint8_t>(raw & 0x3Fu); }

        /// @brief 是否兼容：major 相同即视为兼容（忽略 minor/patch 差异）。
        constexpr bool compatible_with(Version other) const noexcept
        {
            return major() == other.major();
        }

        constexpr bool operator==(Version other) const noexcept { return raw == other.raw; }
        constexpr bool operator!=(Version other) const noexcept { return raw != other.raw; }
    };

    /// @brief 比较两个原始 uint16_t 版本号是否兼容（仅比较 major）。
    constexpr bool versions_compatible(uint16_t a, uint16_t b) noexcept
    {
        return ((a >> 12) & 0x0Fu) == ((b >> 12) & 0x0Fu);
    }

    /// @brief 比较两个 Version 是否兼容（仅比较 major）。
    constexpr bool versions_compatible(Version a, Version b) noexcept
    {
        return a.compatible_with(b);
    }

    /// @brief 当前协议版本号（库编译时版本）。
    inline constexpr Version CURRENT_PROTOCOL_VERSION{1, 0, 0};

} // namespace florid::protocol

#endif // FLORID_PROTOCOLS_VERSION_HPP
