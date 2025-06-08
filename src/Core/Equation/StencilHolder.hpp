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

#ifndef OPFLOW_STENCILHOLDER_HPP
#define OPFLOW_STENCILHOLDER_HPP

#include "Core/Equation/EquationHolder.hpp"
#include "Core/Macros.hpp"
#include "DataStructures/StencilPad.hpp"

OPFLOW_MODULE_EXPORT namespace OpFlow {
    template <typename... E>
    struct StencilHolder;

    template <typename... Es, typename... Ts>
    struct StencilHolder<EquationSet<Es...>, TargetSet<Ts...>> {
        constexpr static int size = sizeof...(Es);

        std::vector<std::any> targets, eqns;
        template <int i>
        using equation_type = typename EquationSet<Es...>::template eqn_type<i>;
        template <int i>
        using eqn_expr_type
                = decltype(std::declval<equation_type<i>&>().lhs - std::declval<equation_type<i>&>().rhs);
        template <int i>
        using target_type = typename TargetSet<Ts...>::template target_type<i>;
        using stencil_type = typename internal::ExprTrait<eqn_expr_type<0>>::elem_type;
        std::array<stencil_type, size> comm_stencils;

        StencilHolder(EqnHolder<EquationSet<Es...>, TargetSet<Ts...>>& eqnHolder)
            : targets(eqnHolder.targets) {
            std::tuple<Ts*...> target_ptrs;
            Meta::static_for<size>([&]<int i>(Meta::int_<i>) {
                std::get<i>(target_ptrs) = eqnHolder.template getTargetPtr<i>();
            });
            Meta::static_for<size>([&]<int i>(Meta::int_<i>) {
                eqns.emplace_back(std::make_any<eqn_expr_type<i>>(eqnHolder.template getEqnExpr<i>()));
            });

            init_comm_stencils();
        }

        template <int i>
        auto& getEqnExpr() {
            return std::any_cast<eqn_expr_type<i>&>(eqns[i]);
        }

        template <int i>
        auto getTargetPtr() {
            return std::any_cast<target_type<i>*>(targets[i]);
        }

        void init_comm_stencils() {
            Meta::static_for<size>([&]<int i>(Meta::int_<i>) {
                auto st_start = getEqnExpr<i>()[getTargetPtr<i>()->getLocalWritableRange().first()];
                auto st_end = getEqnExpr<i>()[getTargetPtr<i>()->getLocalWritableRange().last()];
                auto st_center = getEqnExpr<i>()[getTargetPtr<i>()->getLocalWritableRange().center()];
                if (st_start.pad.size()
                    == std::max({st_start.pad.size(), st_end.pad.size(), st_center.pad.size()})) {
                    comm_stencils[i] = st_start;
                } else if (st_end.pad.size()
                           == std::max({st_start.pad.size(), st_end.pad.size(), st_center.pad.size()})) {
                    comm_stencils[i] = st_end;
                } else {
                    comm_stencils[i] = st_center;
                }
            });
        }
    };

    template <typename... Es, typename... Ts>
    auto makeStencilHolder(EqnHolder<EquationSet<Es...>, TargetSet<Ts...>>& eqnHolder) {
        return StencilHolder<EquationSet<Es...>, TargetSet<Ts...>>(eqnHolder);
    }
}// namespace OpFlow
#endif