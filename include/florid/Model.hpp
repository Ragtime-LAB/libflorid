#ifndef FLORID_MODEL_HPP
#define FLORID_MODEL_HPP

#include <cstdint>
#include <florid/ArmConfig.hpp>

namespace florid
{
    enum class Frame : uint8_t
    {
        kJoint1 = 1,
        kJoint2,
        kJoint3,
        kJoint4,
        kJoint5,
        kJoint6,
        kFlange = 6,
        kEndEffector = 6,
    };

    class Model
    {
    public:
        static constexpr int kDOF = 6;

        Model(const ArmConfig& config);

        Model(const float a[6],
              const float alpha[6],
              const float d[6],
              const float theta_offset[6] = nullptr);

        ~Model() = default;

        Model(const Model&) = default;
        Model& operator=(const Model&) = default;

        void forwardKinematics(const float* q, float* T_out) const;

        void pose(Frame frame, const float* q, float* T_out) const;

        void bodyJacobian(const float* q, float* J_out) const;

        void zeroJacobian(const float* q, float* J_out) const;

    private:
        float m_a[6];
        float m_alpha[6];
        float m_d[6];
        float m_theta_offset[6];
        float m_cos_alpha[6];
        float m_sin_alpha[6];

        struct TmpFK
        {
            float data[112]; // 7 * 16 floats for T_0_0..T_0_6
        };
        void computeFK(const float* q, TmpFK& out) const;
    };

} // namespace florid

#endif // FLORID_MODEL_HPP
