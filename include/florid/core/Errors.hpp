#ifndef FLORID_ERRORS_HPP
#define FLORID_ERRORS_HPP

#include <cstdint>

namespace florid
{
    struct Errors
    {
        uint32_t raw{0};

        static constexpr uint32_t kJointPositionLimitsViolation       = 1u << 0;
        static constexpr uint32_t kCartesianPositionLimitsViolation   = 1u << 1;
        static constexpr uint32_t kSelfCollisionAvoidanceViolation    = 1u << 2;
        static constexpr uint32_t kJointVelocityViolation             = 1u << 3;
        static constexpr uint32_t kCartesianVelocityViolation         = 1u << 4;
        static constexpr uint32_t kForceControlSafetyViolation        = 1u << 5;
        static constexpr uint32_t kJointReflex                       = 1u << 6;
        static constexpr uint32_t kCartesianReflex                   = 1u << 7;
        static constexpr uint32_t kCommunicationConstraintsViolation   = 1u << 8;
        static constexpr uint32_t kEmergencyStop                      = 1u << 9;
        static constexpr uint32_t kWatchdogTimeout                    = 1u << 10;

        operator bool() const noexcept { return raw != 0; }

        Errors operator|(Errors other) const noexcept
        {
            Errors r;
            r.raw = raw | other.raw;
            return r;
        }

        Errors operator&(Errors other) const noexcept
        {
            Errors r;
            r.raw = raw & other.raw;
            return r;
        }

        Errors& operator|=(Errors other) noexcept { raw |= other.raw; return *this; }
        Errors& operator&=(Errors other) noexcept { raw &= other.raw; return *this; }

        bool operator==(Errors other) const noexcept { return raw == other.raw; }
        bool operator!=(Errors other) const noexcept { return raw != other.raw; }
    };

} // namespace florid

#endif // FLORID_ERRORS_HPP
