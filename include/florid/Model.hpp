#ifndef FLORID_MODEL_HPP
#define FLORID_MODEL_HPP

#include "florid/Frame.hpp"

namespace florid {

template <typename Traits>
class Model {
public:
    static constexpr int s_kDOF = Traits::kDOF;

    Model() = default;
    ~Model() = default;

    void forwardKinematics(const float* s_q, float* s_T_out) const {
        Traits::fk(s_q, s_T_out);
    }

    void pose(Frame s_frame, const float* s_q, float* s_T_out) const {
        Traits::pose(static_cast<int>(s_frame), s_q, s_T_out);
    }

    void zeroJacobian(const float* s_q, float* s_J_out) const {
        Traits::zeroJacobian(s_q, s_J_out);
    }

    void bodyJacobian(const float* s_q, float* s_J_out) const {
        Traits::bodyJacobian(s_q, s_J_out);
    }

    template <bool Enable = Traits::kHasMass>
    void mass(const float* s_q, float* s_M_out) const {
        static_assert(Enable, "mass() not available for this Traits");
        Traits::mass(s_q, s_M_out);
    }

    template <bool Enable = Traits::kHasCoriolis>
    void coriolis(const float* s_q, const float* s_dq, float* s_C_out) const {
        static_assert(Enable, "coriolis() not available for this Traits");
        Traits::coriolis(s_q, s_dq, s_C_out);
    }

    void gravity(const float* s_q, const float s_g_vec[3], float* s_g_out) const {
        Traits::gravity(s_q, s_g_vec, s_g_out);
    }
};

} // namespace florid

#endif // FLORID_MODEL_HPP
