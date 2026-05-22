#ifndef FLORID_TYPES_HPP
#define FLORID_TYPES_HPP

#include <cstdint>
#include <florid/protocols/protocols.hpp>

namespace florid
{
    enum class ArmMode : uint8_t
    {
        Init = 0,
        Idle = 1,
        Running = 2,
        Fault = 3,
        EStop = 4,
    };

    enum class ControllerMode : uint8_t
    {
        JointImpedance = 0,
        CartesianImpedance = 1,
    };

    struct ArmState
    {
        double       time{};
        ArmMode    mode{};
        uint32_t     errors{};
        float        q[6]{};
        float        dq[6]{};
        float        tau[6]{};
        float        tau_desired[6]{};
        float        O_T_EE[16]{};
        float        F_ext[6]{};
        float        base_gravity[3]{};
    };

    struct Finishable
    {
        bool motion_finished{false};
    };

    struct Torques : Finishable
    {
        float tau[6]{};

        static Torques MotionFinished(const Torques& command)
        {
            Torques result = command;
            result.motion_finished = true;
            return result;
        }
    };

    struct JointPositions : Finishable
    {
        float q[6]{};

        static JointPositions MotionFinished(const JointPositions& command)
        {
            JointPositions result = command;
            result.motion_finished = true;
            return result;
        }
    };

    struct JointVelocities : Finishable
    {
        float dq[6]{};

        static JointVelocities MotionFinished(const JointVelocities& command)
        {
            JointVelocities result = command;
            result.motion_finished = true;
            return result;
        }
    };

    struct CartesianPose : Finishable
    {
        float T[16]{};

        bool hasElbow() const { return false; }

        static CartesianPose MotionFinished(const CartesianPose& command)
        {
            CartesianPose result = command;
            result.motion_finished = true;
            return result;
        }
    };

    struct CartesianVelocities : Finishable
    {
        float v[6]{};

        static CartesianVelocities MotionFinished(const CartesianVelocities& command)
        {
            CartesianVelocities result = command;
            result.motion_finished = true;
            return result;
        }
    };
}

#endif //FLORID_TYPES_HPP
