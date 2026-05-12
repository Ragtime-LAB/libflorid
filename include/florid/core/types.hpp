#ifndef FLORID_TYPES_HPP
#define FLORID_TYPES_HPP

#include <florid/protocols/protocols.hpp>

namespace florid::core
{
    using JointCommand = protocol::JointState;
    using CartesianPose = protocol::CartesianPose;

    struct ArmStatus
    {
        protocol::ArmMode mode;
        protocol::JointState state;
    };
}

#endif //FLORID_TYPES_HPP
