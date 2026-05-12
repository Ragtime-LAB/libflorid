#ifndef FLORID_TRAITS_HPP
#define FLORID_TRAITS_HPP

#include "types.hpp"

namespace florid::core
{
    template <bool Value>
    struct bool_constant
    {
        static constexpr bool value = Value;
    };

    using true_type = bool_constant<true>;
    using false_type = bool_constant<false>;

    template <typename T>
    struct remove_reference
    {
        using type = T;
    };

    template <typename T>
    struct remove_reference<T&>
    {
        using type = T;
    };

    template <typename T>
    struct remove_reference<T&&>
    {
        using type = T;
    };

    template <typename T>
    struct remove_cv
    {
        using type = T;
    };

    template <typename T>
    struct remove_cv<const T>
    {
        using type = T;
    };

    template <typename T>
    struct remove_cv<volatile T>
    {
        using type = T;
    };

    template <typename T>
    struct remove_cv<const volatile T>
    {
        using type = T;
    };

    template <typename T>
    struct decay_simple
    {
        using type = typename remove_cv<typename remove_reference<T>::type>::type;
    };

    template <typename T>
    struct is_control_command_impl : false_type
    {
    };

    template <>
    struct is_control_command_impl<JointCommand> : true_type
    {
    };

    template <>
    struct is_control_command_impl<CartesianPose> : true_type
    {
    };

    template <typename T>
    struct is_control_command : is_control_command_impl<typename decay_simple<T>::type>
    {
    };
}

#endif //FLORID_TRAITS_HPP
