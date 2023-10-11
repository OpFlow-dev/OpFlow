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

#ifndef OPFLOW_EQUATIONHOLDER_HPP
#define OPFLOW_EQUATIONHOLDER_HPP

#include "Core/Equation/Equation.hpp"
#include "Core/Meta.hpp"
#include "DataStructures/StencilPad.hpp"
#include <any>
#include <boost/core/demangle.hpp>
#include <functional>
#include <unordered_map>
#include <vector>

namespace OpFlow {
    template <typename E, typename T>
    struct EqnHolder;

    template <typename... Es, typename... Ts>
    struct EqnHolder<EquationSet<Es...>, TargetSet<Ts...>> {
        static_assert(sizeof...(Es) == sizeof...(Ts));
        constexpr static int size = sizeof...(Es);

        using eqns_type = EquationSet<Es...>;
        using targets_type = TargetSet<Ts...>;

        template <int i>
        using st_field_type = std::conditional_t<
                (size > 1),
                Meta::RealType<decltype(std::declval<typename targets_type::template target_type<i>*>()
                                                ->template getStencilField<std::unordered_map>(i))>,
                Meta::RealType<decltype(std::declval<typename targets_type::template target_type<i>*>()
                                                ->template getStencilField<DS::fake_map_default>(i))>>;

        template <int i, typename Is>
        struct getter_helper;
        template <int i, std::size_t... Ints>
        struct getter_helper<i, std::index_sequence<Ints...>> {
            using type = std::function<typename eqns_type ::template eqn_type<i>(st_field_type<Ints>&...)>;
        };
        template <int i, typename Is = std::make_index_sequence<size>>
        using getter_type = typename getter_helper<i, Is>::type;

        // containers of instance pointers
        std::vector<std::any> getters, targets, st_fields;

        template <typename... Fs>
        EqnHolder(std::tuple<Fs...>& getters, std::tuple<Ts&...>& targets) {
            Meta::static_for<size>([&]<int i>(Meta::int_<i>) {
                this->getters.push_back(getter_type<i>(std::get<i>(getters)));
                this->targets.push_back(&std::get<i>(targets));
                if constexpr (size == 1)
                    this->st_fields.push_back((std::make_shared<st_field_type<i>>(
                            std::any_cast<typename targets_type::template target_type<i>*>(this->targets[i])
                                    ->template getStencilField<DS::fake_map_default>(i))));
                else
                    this->st_fields.push_back((std::make_shared<st_field_type<i>>(
                            std::any_cast<typename targets_type::template target_type<i>*>(this->targets[i])
                                    ->template getStencilField<std::unordered_map>(i))));
            });
        }

        template <int i>
        auto getTargetPtr() {
            return std::any_cast<typename targets_type::template target_type<i>*>(targets[i]);
        }

        template <int i>
        auto& getGetter() {
            return std::any_cast<getter_type<i>&>(this->getters[i]);
        }

        template <int i>
        auto getEqnExpr() {
            auto k = i;
            auto s = this->st_fields.size();
            auto eqn = Meta::custom_apply_container<size>(
                    getGetter<i>(),
                    [&]<int k>(std::any & st_ptr, Meta::int_<k>) -> decltype(auto) {
                        return *(std::any_cast<std::shared_ptr<st_field_type<k>>&>(st_ptr));
                    },
                    st_fields);
            auto t = eqn.lhs - eqn.rhs;
            t.prepare();
            return t;
        }
    };

    template <typename... Fs, typename... Ts, std::size_t... Ints>
    auto makeEqnHolder_impl(std::tuple<Fs...>&& getters, std::tuple<Ts&...>&& targets,
                            std::index_sequence<Ints...>) {
        if constexpr (sizeof...(Ts) == 1)
            return EqnHolder<EquationSet<decltype(std::declval<Meta::RealType<Fs>>()(
                                     std::declval<Meta::RealType<Ts>&>()
                                             .template getStencilField<DS::fake_map_default>(Ints)...))...>,
                             TargetSet<Meta::RealType<Ts>...>>(getters, targets);
        else
            return EqnHolder<EquationSet<decltype(std::declval<Meta::RealType<Fs>>()(
                                     std::declval<Meta::RealType<Ts>&>()
                                             .template getStencilField<std::unordered_map>(Ints)...))...>,
                             TargetSet<Meta::RealType<Ts>...>>(getters, targets);
    }
    template <typename... Fs, typename... Ts, std::size_t... Ints>
    auto makeEqnHolder_impl(std::tuple<Fs...>& getters, std::tuple<Ts&...>& targets,
                            std::index_sequence<Ints...>) {
        if constexpr (sizeof...(Ts) == 1)
            return EqnHolder<EquationSet<decltype(std::declval<Meta::RealType<Fs>>()(
                                     std::declval<Meta::RealType<Ts>&>()
                                             .template getStencilField<DS::fake_map_default>(Ints)...))...>,
                             TargetSet<Meta::RealType<Ts>...>>(getters, targets);
        else
            return EqnHolder<EquationSet<decltype(std::declval<Meta::RealType<Fs>&>()(
                                     std::declval<Meta::RealType<Ts>&>()
                                             .template getStencilField<std::unordered_map>(Ints)...))...>,
                             TargetSet<Meta::RealType<Ts>...>>(getters, targets);
    }

    template <typename... Fs, typename... Ts>
    auto makeEqnHolder(std::tuple<Fs...>& getters, std::tuple<Ts&...>& targets) {
        return makeEqnHolder_impl(OP_PERFECT_FOWD(getters), OP_PERFECT_FOWD(targets),
                                  std::make_index_sequence<sizeof...(Fs)>());
    }
    template <typename... Fs, typename... Ts>
    auto makeEqnHolder(std::tuple<Fs...>&& getters, std::tuple<Ts&...>&& targets) {
        return makeEqnHolder_impl(OP_PERFECT_FOWD(getters), OP_PERFECT_FOWD(targets),
                                  std::make_index_sequence<sizeof...(Fs)>());
    }
}// namespace OpFlow

#endif