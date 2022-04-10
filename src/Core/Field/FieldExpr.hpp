// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2022 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_FIELDEXPR_HPP
#define OPFLOW_FIELDEXPR_HPP

#include "Core/Constants.hpp"
#include "Core/Expr/Expr.hpp"
#include "Core/Expr/ExprTrait.hpp"
#include "Core/Field/FieldExprTrait.hpp"
#include "Core/Meta.hpp"

namespace OpFlow {

    template <typename Derived>
    struct FieldExpr;

    template <typename Derived>
    struct FieldExpr : Expr<Derived> {
        static constexpr int dim = internal::FieldExprTrait<Derived>::dim;
        static constexpr int getDim() { return dim; }

    protected:
        // recurse end
        void initPropsFromImpl_FieldExpr(auto&& expr) const {
            if (this->name.empty()) this->name = expr.getName();
        }

    private:
        constexpr static auto access_flag = internal::FieldExprTrait<Derived>::access_flag;
        DEFINE_CRTP_HELPERS(Derived)
    };

}// namespace OpFlow
#endif//OPFLOW_FIELDEXPR_HPP
