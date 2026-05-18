#ifndef FLORID_ARMCONFIG_HPP
#define FLORID_ARMCONFIG_HPP

#include "util/math.hpp"
#include <cstdint>

namespace florid
{
    struct ArmConfig
    {
        static constexpr int kDOF = 6;

        float a[kDOF]            {};
        float alpha[kDOF]        {};
        float d[kDOF]            {};
        float theta_offset[kDOF] {};

        float cos_alpha[kDOF] {};
        float sin_alpha[kDOF] {};

        float collision_lower[kDOF] {};
        float collision_upper[kDOF] {};
        float joint_impedance[kDOF]  {};
        float cartesian_impedance[kDOF] {};

        float EE_T_K[16]  {};
        float NE_T_EE[16] {};

        float load_mass          {};
        float load_center[3]     {};
        float load_inertia[9]    {};

        uint16_t watchdog_timeout_ms {500};
    };

    // ── 工厂 ────────────────────────────────────────────────────
    constexpr ArmConfig MakeArmConfig(
        const float* a,       const float* alpha,
        const float* d,       const float* theta_offset,
        const float* cl,      const float* cu,
        const float* jimp,    const float* cimp)
    {
        ArmConfig cfg{};
        for (int i = 0; i < ArmConfig::kDOF; ++i)
        {
            cfg.a[i]             = a[i];
            cfg.alpha[i]         = alpha[i];
            cfg.d[i]             = d[i];
            cfg.theta_offset[i]  = theta_offset ? theta_offset[i] : 0.0f;
            cfg.collision_lower[i] = cl[i];
            cfg.collision_upper[i] = cu[i];
            cfg.joint_impedance[i]  = jimp[i];
            cfg.cartesian_impedance[i] = cimp[i];

            using namespace florid::comptime;
            cfg.cos_alpha[i] = cosf(alpha[i]);
            cfg.sin_alpha[i] = sinf(alpha[i]);
        }
        return cfg;
    }

    // ══════════════════════════════════════════════════════════════
    //  预置臂型
    // ══════════════════════════════════════════════════════════════

    namespace armconfig_detail
    {
        constexpr float _0[6] {};
        constexpr float _coll_low_dflt[6] = {20,20,20,20,10,10};
        constexpr float _coll_up_dflt[6]  = {20,20,20,20,10,10};
        constexpr float _jimp_dflt[6]     = {3000,3000,3000,2500,2500,2000};
        constexpr float _cimp_dflt[6]     = {600,600,600,30,30,30};

        constexpr float _panda_a[6]     = {0, 0, 0, 0.0825f, -0.0825f, 0};
        constexpr float _panda_alpha[6] = {0, -1.571f, 1.571f, 1.571f, -1.571f, 1.571f};
        constexpr float _panda_d[6]     = {0.333f, 0, 0.316f, 0, 0.384f, 0.107f};
    }

    constexpr ArmConfig kEmptyConfig = MakeArmConfig(
        armconfig_detail::_0, armconfig_detail::_0, armconfig_detail::_0, nullptr,
        armconfig_detail::_coll_low_dflt, armconfig_detail::_coll_up_dflt,
        armconfig_detail::_jimp_dflt, armconfig_detail::_cimp_dflt);

    constexpr ArmConfig kPandaConfig = MakeArmConfig(
        armconfig_detail::_panda_a, armconfig_detail::_panda_alpha,
        armconfig_detail::_panda_d, nullptr,
        armconfig_detail::_coll_low_dflt, armconfig_detail::_coll_up_dflt,
        armconfig_detail::_jimp_dflt, armconfig_detail::_cimp_dflt);

} // namespace florid

#endif // FLORID_ARMCONFIG_HPP
