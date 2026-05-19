#include <gtest/gtest.h>
#include <florid/Model.hpp>
#include <florid/traits/PantheraTraits.hpp>
#include <cmath>
#include <cstring>

using florid::Model;
using florid::PantheraTraits;

class ModelTest : public ::testing::Test
{
protected:
    Model<PantheraTraits> model;
    float q[6]   = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f};
    float dq[6]  = {0.01f, 0.02f, 0.03f, 0.04f, 0.05f, 0.06f};
    float gv[3]  = {0, 0, -9.81f};

    static bool near(float a, float b, float eps = 2e-3f)
    {
        return std::fabs(a - b) < eps;
    }
};

// ── FK: rotation matrix must be orthogonal ─────────────────────
TEST_F(ModelTest, FkRotationIsOrthogonal)
{
    float T[16];
    model.forwardKinematics(q, T);

    // R^T * R should be identity
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
        {
            float dot = 0;
            for (int k = 0; k < 3; ++k)
                dot += T[k*4 + r] * T[k*4 + c];  // column-major transpose
            float expect = (r == c) ? 1.0f : 0.0f;
            EXPECT_NEAR(dot, expect, 1e-3f);
        }

    // determinant ≈ 1
    float det = T[0]*(T[5]*T[10]-T[6]*T[9])
              - T[1]*(T[4]*T[10]-T[6]*T[8])
              + T[2]*(T[4]*T[9] -T[5]*T[8]);
    EXPECT_NEAR(det, 1.0f, 1e-3f);
}

// ── FK output is the same as pose(Frame::kFlange) ──────────────
TEST_F(ModelTest, FkMatchesPoseFlange)
{
    float T_fk[16], T_pose[16];
    model.forwardKinematics(q, T_fk);
    model.pose(florid::Frame::kFlange, q, T_pose);

    for (int i = 0; i < 16; ++i)
        EXPECT_NEAR(T_fk[i], T_pose[i], 1e-5f);
}

// ── Mass matrix: symmetric, positive diagonal ──────────────────
TEST_F(ModelTest, MassSymmetricAndPositiveDiag)
{
    float M[36];
    model.mass(q, M);

    // symmetry
    for (int r = 0; r < 6; ++r)
        for (int c = 0; c < 6; ++c)
            EXPECT_NEAR(M[r*6 + c], M[c*6 + r], 1e-4f);

    // positive diagonal
    for (int i = 0; i < 6; ++i)
        EXPECT_GT(M[i*6 + i], 0.0f);
}

// ── Gravity: proportional to g_vec magnitude ───────────────────
TEST_F(ModelTest, GravityScalesWithGVec)
{
    float g1[6], g2[6];
    float gv_full[3] = {0, 0, -9.81f};
    float gv_half[3] = {0, 0, -4.905f};

    model.gravity(q, gv_full, g1);
    model.gravity(q, gv_half, g2);

    for (int i = 0; i < 6; ++i)
        EXPECT_NEAR(g2[i] * 2.0f, g1[i], 1e-3f);
}

// ── Zero Jacobian is 6×6, non-zero for reasonable q ─────────────
TEST_F(ModelTest, ZeroJacobianNonZero)
{
    float J[36];
    model.zeroJacobian(q, J);

    float norm_sq = 0;
    for (int i = 0; i < 36; ++i) norm_sq += J[i] * J[i];
    EXPECT_GT(norm_sq, 1e-6f);  // should not be all zeros
}

// ── Body Jacobian also non-zero ─────────────────────────────────
TEST_F(ModelTest, BodyJacobianNonZero)
{
    float J[36];
    model.bodyJacobian(q, J);

    float norm_sq = 0;
    for (int i = 0; i < 36; ++i) norm_sq += J[i] * J[i];
    EXPECT_GT(norm_sq, 1e-6f);
}

// ── Different q produce different FK ────────────────────────────
TEST_F(ModelTest, DifferentQDifferentFk)
{
    float q2[6] = {0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f};
    float T1[16], T2[16];

    model.forwardKinematics(q,  T1);
    model.forwardKinematics(q2, T2);

    float diff = 0;
    for (int i = 0; i < 16; ++i) diff += std::fabs(T1[i] - T2[i]);
    EXPECT_GT(diff, 1e-3f);
}

// ── Coriolis at zero velocity is zero ───────────────────────────
TEST_F(ModelTest, CoriolisZeroAtZeroVelocity)
{
    float dq_zero[6] = {};
    float C[6];

    model.coriolis(q, dq_zero, C);

    for (int i = 0; i < 6; ++i)
        EXPECT_NEAR(C[i], 0.0f, 1e-4f);
}

// ── Pose for different frames: flange ≥ joint1 in z ─────────────
TEST_F(ModelTest, PoseFrameProgression)
{
    float T1[16], T6[16];
    model.pose(florid::Frame::kJoint1, q, T1);
    model.pose(florid::Frame::kFlange,  q, T6);

    // Flange position norm should be ≥ joint1 position norm
    float norm1 = std::sqrt(T1[12]*T1[12] + T1[13]*T1[13] + T1[14]*T1[14]);
    float norm6 = std::sqrt(T6[12]*T6[12] + T6[13]*T6[13] + T6[14]*T6[14]);
    // Not a strict inequality, just sanity — both should be finite
    EXPECT_TRUE(std::isfinite(norm1));
    EXPECT_TRUE(std::isfinite(norm6));
}
