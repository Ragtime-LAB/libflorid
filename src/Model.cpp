#include <florid/Model.hpp>
#include <BasicLinearAlgebra.h>
#include <math.h>
#include <cstring>

namespace florid
{
    namespace
    {
        using BLA::Matrix;

        inline float cosf_w(float x) { return cosf(x); }
        inline float sinf_w(float x) { return sinf(x); }
    }

    Model::Model(const ArmConfig& cfg)
    {
        for (int i = 0; i < 6; ++i)
        {
            m_a[i]     = cfg.a[i];
            m_alpha[i] = cfg.alpha[i];
            m_d[i]     = cfg.d[i];
            m_theta_offset[i] = cfg.theta_offset[i];
            m_cos_alpha[i] = cfg.cos_alpha[i];
            m_sin_alpha[i] = cfg.sin_alpha[i];
        }
    }

    Model::Model(const float a[6],
                 const float alpha[6],
                 const float d[6],
                 const float theta_offset[6])
    {
        for (int i = 0; i < 6; ++i)
        {
            m_a[i]     = a[i];
            m_alpha[i] = alpha[i];
            m_d[i]     = d[i];
            m_theta_offset[i] = theta_offset ? theta_offset[i] : 0.0f;
            m_cos_alpha[i] = cosf_w(m_alpha[i]);
            m_sin_alpha[i] = sinf_w(m_alpha[i]);
        }
    }

    void Model::computeFK(const float* q, TmpFK& out) const
    {
        Matrix<4,4> Ts[7];
        Ts[0] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};

        for (int i = 0; i < 6; ++i)
        {
            float theta = q[i] + m_theta_offset[i];
            float ct = cosf_w(theta);
            float st = sinf_w(theta);
            float ca = m_cos_alpha[i];
            float sa = m_sin_alpha[i];
            float a  = m_a[i];
            float d  = m_d[i];

            Matrix<4,4> dh = {
                 ct,  -st * ca,   st * sa,  a * ct,
                 st,   ct * ca,  -ct * sa,  a * st,
                 0,    sa,         ca,       d,
                 0,    0,          0,        1
            };

            Ts[i + 1] = Ts[i] * dh;
        }

        float* dst = out.data;
        for (int i = 0; i < 7; ++i) {
            for (int c = 0; c < 4; ++c) {    // column-major
                for (int r = 0; r < 4; ++r) {
                    *dst++ = Ts[i](r, c);
                }
            }
        }
    }

    void Model::forwardKinematics(const float* q, float* T_out) const
    {
        TmpFK fk;
        computeFK(q, fk);
        auto* T_arr = reinterpret_cast<const float*>(&fk);
        // T_0_6 is at offset 6*16
        std::memcpy(T_out, T_arr + 6 * 16, 16 * sizeof(float));
    }

    void Model::pose(Frame frame, const float* q, float* T_out) const
    {
        int idx = static_cast<int>(frame);
        if (idx < 0) idx = 0;
        if (idx > 6) idx = 6;

        TmpFK fk;
        computeFK(q, fk);
        auto* T_arr = reinterpret_cast<const float*>(&fk);
        std::memcpy(T_out, T_arr + idx * 16, 16 * sizeof(float));
    }

    void Model::zeroJacobian(const float* q, float* J_out) const
    {
        TmpFK fk;
        computeFK(q, fk);
        auto* T_arr = reinterpret_cast<const float*>(&fk);

        float p[7][3];
        float z[6][3];

        for (int i = 0; i < 7; ++i)
        {
            const float* Ti = T_arr + i * 16;
            // Column-major: T(index=row+col*4)
            p[i][0] = Ti[12]; p[i][1] = Ti[13]; p[i][2] = Ti[14];
        }

        for (int i = 0; i < 6; ++i)
        {
            const float* Ti = T_arr + i * 16;
            z[i][0] = Ti[8];  // col 2 (indices 8,9,10)
            z[i][1] = Ti[9];
            z[i][2] = Ti[10];
        }

        const float* pn = p[6];
        std::memset(J_out, 0, 36 * sizeof(float));

        for (int i = 0; i < 6; ++i)
        {
            float dp[3] = {
                pn[0] - p[i][0],
                pn[1] - p[i][1],
                pn[2] - p[i][2]
            };

            // J_v: z_i × (p_n - p_i)
            J_out[0  + i] = z[i][1] * dp[2] - z[i][2] * dp[1];
            J_out[6  + i] = z[i][2] * dp[0] - z[i][0] * dp[2];
            J_out[12 + i] = z[i][0] * dp[1] - z[i][1] * dp[0];

            // J_w: z_i
            J_out[18 + i] = z[i][0];
            J_out[24 + i] = z[i][1];
            J_out[30 + i] = z[i][2];
        }
    }

    void Model::bodyJacobian(const float* q, float* J_out) const
    {
        float J_zero[36];
        zeroJacobian(q, J_zero);

        TmpFK fk;
        computeFK(q, fk);
        auto* T_arr = reinterpret_cast<const float*>(&fk);

        // R^T from T_0_6 (column-major -> R^T row-major)
        const float* T6 = T_arr + 6 * 16;
        float RT[9];
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 3; ++c)
                RT[r * 3 + c] = T6[r * 4 + c];  // R^T[r][c] = T_0_6[r][c]

        for (int col = 0; col < 6; ++col)
        {
            float vx = J_zero[0  + col];
            float vy = J_zero[6  + col];
            float vz = J_zero[12 + col];
            float wx = J_zero[18 + col];
            float wy = J_zero[24 + col];
            float wz = J_zero[30 + col];

            J_out[0  + col] = RT[0]*vx + RT[1]*vy + RT[2]*vz;
            J_out[6  + col] = RT[3]*vx + RT[4]*vy + RT[5]*vz;
            J_out[12 + col] = RT[6]*vx + RT[7]*vy + RT[8]*vz;

            J_out[18 + col] = RT[0]*wx + RT[1]*wy + RT[2]*wz;
            J_out[24 + col] = RT[3]*wx + RT[4]*wy + RT[5]*wz;
            J_out[30 + col] = RT[6]*wx + RT[7]*wy + RT[8]*wz;
        }
    }

} // namespace florid
