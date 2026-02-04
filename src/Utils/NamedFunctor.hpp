// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2026 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_NAMEDFUNCTOR_HPP
#define OPFLOW_NAMEDFUNCTOR_HPP

#include "Core/Meta.hpp"
#include "Utils/ConstexprString.hpp"
#ifndef OPFLOW_INSIDE_MODULE
#include <concepts>
#endif

OPFLOW_MODULE_EXPORT namespace OpFlow::Utils {
    template <auto Functor, auto Name, typename... Args>
            // Invokable check. Omit Args to mute this check
            requires((sizeof...(Args) == 0) || std::invocable<Meta::RealType<decltype(Functor)>, Args...>)
            // Name type check. Must be a valid constexpr string
            && (CXprStringType<Meta::RealType<decltype(Name)>>) struct NamedFunctor {
        constexpr NamedFunctor() = default;

        constexpr static auto getFunc() { return _func; }
        constexpr static auto getName() { return _name; }

        constexpr auto operator()(auto&&... args) const { return _func(OP_PERFECT_FOWD(args)...); }

    private:
        constexpr static Meta::RealType<decltype(Functor)> _func = Functor;
        constexpr static Meta::RealType<decltype(Name)> _name = Name;
    };

    namespace internal {
        template <typename T>
        struct is_named_functor : std::false_type {};

        template <auto Functor, auto Name, typename... Args>
        struct is_named_functor<NamedFunctor<Functor, Name, Args...>> : std::true_type {};
    }// namespace internal

    template <typename T>
    concept NamedFunctorType = internal::is_named_functor<T>::value;

    constexpr auto makeNamedFunctor(auto&& func, auto&& name) { return NamedFunctor<func, name>(); }

    constexpr auto makeNamedFunctor(auto&& func) {
        return NamedFunctor<func, makeCXprString("unnamed_functor")>();
    }
}// namespace OpFlow::Utils
#endif//OPFLOW_NAMEDFUNCTOR_HPP
