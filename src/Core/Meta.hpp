//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2023 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_META_HPP
#define OPFLOW_META_HPP

#ifndef OPFLOW_INSIDE_MODULE
#include <array>
#include <concepts>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>
#endif

namespace OpFlow::Meta {

    template <int k>
    struct int_ : std::integral_constant<int, k> {};
    template <bool b>
    struct bool_ : std::integral_constant<bool, b> {};
    /*    template <float f>
    struct float_ : std::integral_constant<float, f> {};
    template <double d>
    struct double_ : std::integral_constant<double, d> {};*/

    template <template <typename...> typename, typename...>
    struct isTemplateInstance : public std::false_type {};

    template <template <typename...> typename U, typename... T>
    struct isTemplateInstance<U, U<T...>> : public std::true_type {};

    template <typename F>
    using RealType = std::remove_cvref_t<F>;

    template <typename T>
    struct firstParamType;

    template <typename R, typename P1>
    struct firstParamType<R(P1)> {
        using type = P1;
    };

    template <typename... T>
    struct firstOf {
        using type = typename std::tuple_element<0, std::tuple<T...>>::type;
    };

    template <typename... Ts>
    using firstOf_t = typename firstOf<Ts...>::type;

    template <typename... T>
    struct lastOf {
        // last type in the type list
        using type = typename std::tuple_element<sizeof...(T) - 1, std::tuple<T...>>::type;
    };

    template <typename... T>
    using lastOf_t = typename lastOf<T...>::type;

    template <typename T>
    struct packedType;

    template <>
    struct packedType<int> {
        using type = int;
    };

    template <>
    struct packedType<double> {
        using type = double;
    };

    template <>
    struct packedType<float> {
        using type = float;
    };

    template <typename T>
    struct is_numerical {
        constexpr static bool value = std::is_integral_v<T> || std::is_floating_point_v<T>;
    };

    template <typename T>
    inline constexpr bool is_numerical_v = is_numerical<T>::value;

    template <typename T>
    concept Numerical = is_numerical_v<Meta::RealType<T>>;

    template <typename T>
    concept WeakIntegral = std::integral<typename std::remove_cvref<T>::type>;

    template <typename T>
    concept StdRatio = requires {
        { T::num } -> std::convertible_to<int>;
        { T::den } -> std::convertible_to<int>;
        typename T::type;
    };

    template <typename T>
    struct TypeWrapper {
        using type = T;
    };

    template <typename T>
    concept BracketIndexable = requires(T t) { t[0]; };

    template <bool b>
    struct bool_type : std::false_type {};

    template <>
    struct bool_type<true> : std::true_type {};

    template <typename...>
    struct TypeName;

    template <typename F, int... Is>
    void static_for(F&& func, std::integer_sequence<int, Is...>) {
        (func(int_<Is> {}), ...);
    }

    template <std::size_t N, typename F>
    void static_for(F&& func) {
        static_for(std::forward<F>(func), std::make_integer_sequence<int, N>());
    }

    template <typename, typename, typename>
    struct integer_seq_cat;

    template <typename T, T... As, T... Bs>
    struct integer_seq_cat<T, std::integer_sequence<T, As...>, std::integer_sequence<T, Bs...>> {
        using type = std::integer_sequence<T, As..., Bs...>;
    };

    template <typename T, T start, T end, T step = 1>
    struct make_integer_seq_impl;

    template <typename T, T start, T end, T step>
        requires(step > 0 && start < end) || (step<0 && start> end)
    struct make_integer_seq_impl<T, start, end, step> {
        using type = typename integer_seq_cat<
                T, std::integer_sequence<T, start>,
                typename make_integer_seq_impl<T, start + step, end, step>::type>::type;
    };

    template <typename T, T start, T end, T step>
        requires(step > 0 && start >= end) || (step < 0 && start <= end)
    struct make_integer_seq_impl<T, start, end, step> {
        using type = std::integer_sequence<T>;
    };

    template <typename T, T start, T end, T step = 1>
    using make_integer_sequence = typename make_integer_seq_impl<T, start, end, step>::type;

    template <int start, int end, typename F>
    void static_for(F&& func) {
        static_for(std::forward<F>(func), make_integer_sequence<int, start, end>());
    }

    template <int start, int end, int step, typename F>
    void static_for(F&& func) {
        static_for(std::forward<F>(func), make_integer_sequence<int, start, end, step>());
    }

    template <std::size_t... Ints, std::size_t... IntsRemain, typename... Ts>
    inline auto tuple_split_impl(std::index_sequence<Ints...>, std::index_sequence<IntsRemain...>,
                                 std::tuple<Ts...>& t) {
        return std::make_tuple(std::forward_as_tuple(std::get<Ints>(t)...),
                               std::forward_as_tuple(std::get<IntsRemain>(t)...));
    }

    template <std::size_t Nsplit, typename... Ts>
    inline auto tuple_split(std::tuple<Ts...>& t) {
        static_assert(Nsplit < sizeof...(Ts));
        return tuple_split_impl(std::make_index_sequence<Nsplit>(),
                                make_integer_sequence<std::size_t, Nsplit, sizeof...(Ts)>(), t);
    }

    template <typename F, typename G, typename... Ts>
    constexpr decltype(auto) custom_invoke(F&& func, G&& getter, Ts&&... ts) {
        return std::invoke(std::forward<F>(func), getter(std::forward<Ts>(ts))...);
    }

    template <typename F, typename G, typename Tuple, std::size_t... Ints>
    constexpr decltype(auto) custom_apply_impl(F&& func, G&& getter, Tuple&& tuple,
                                               std::index_sequence<Ints...>) {
        return custom_invoke(std::forward<F>(func), std::forward<G>(getter),
                             std::get<Ints>(std::forward<Tuple>(tuple))...);
    }

    template <typename F, typename G, typename Tuple>
    constexpr decltype(auto) custom_apply(F&& func, G&& getter, Tuple&& tuple) {
        return custom_apply_impl(std::forward<F>(func), std::forward<G>(getter), std::forward<Tuple>(tuple),
                                 std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<Tuple>>>());
    }

    template <typename F, typename G, typename C, std::size_t... Ints>
    constexpr decltype(auto) custom_apply_container_impl(F&& func, G&& getter, C&& container,
                                                         std::index_sequence<Ints...>) {
        return std::invoke(std::forward<F>(func), getter(container[Ints], Meta::int_<Ints>())...);
    }

    template <std::size_t n, typename F, typename G, typename C>
    constexpr decltype(auto) custom_apply_container(F&& func, G&& getter, C&& container) {
        return custom_apply_container_impl(std::forward<F>(func), std::forward<G>(getter),
                                           std::forward<C>(container), std::make_index_sequence<n>());
    }
}// namespace OpFlow::Meta
#endif//OPFLOW_META_HPP
