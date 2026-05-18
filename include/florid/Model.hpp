#ifndef FLORID_MODEL_HPP
#define FLORID_MODEL_HPP

#include <florid/Frame.hpp>

namespace florid
{
    template <typename Traits>
    class Model
    {
    public:
        static constexpr int kDOF = Traits::kDOF;

        Model() = default;
        ~Model() = default;

        // ── 运动学 ────────────────────────────────────────────

        void forwardKinematics(const float* q, float* T_out) const
        {
            Traits::fk(q, T_out);
        }

        void pose(Frame frame, const float* q, float* T_out) const
        {
            Traits::pose(static_cast<int>(frame), q, T_out);
        }

        void zeroJacobian(const float* q, float* J_out) const
        {
            Traits::zeroJacobian(q, J_out);
        }

        void bodyJacobian(const float* q, float* J_out) const
        {
            Traits::bodyJacobian(q, J_out);
        }

        // ── 动力学 ────────────────────────────────────────────

        template <bool Enable = Traits::kHasMass>
        void mass(const float* q, float* M_out) const
        {
            static_assert(Enable, "mass() not available for this Traits");
            Traits::mass(q, M_out);
        }

        template <bool Enable = Traits::kHasCoriolis>
        void coriolis(const float* q, const float* dq, float* C_out) const
        {
            static_assert(Enable, "coriolis() not available for this Traits");
            Traits::coriolis(q, dq, C_out);
        }

        void gravity(const float* q, const float g_vec[3], float* g_out) const
        {
            Traits::gravity(q, g_vec, g_out);
        }
    };

} // namespace florid

#endif // FLORID_MODEL_HPP
