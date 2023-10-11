// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2023 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_COMPARE_HPP
#define OPFLOW_COMPARE_HPP

#include "Core/Expr/Expr.hpp"
#include "Core/Expr/Expression.hpp"
#include "Core/Meta.hpp"
#include "Core/Operator/BinOpDefMacros.hpp.in"
#include "Core/Operator/Operator.hpp"

#include <cmath>

namespace OpFlow {
    DEFINE_BINOP(GreaterThan, >)
    DEFINE_BINOP(GreaterThanOrEqual, >=)
    DEFINE_BINOP(LessThan, <)
    DEFINE_BINOP(LessThanOrEqual, <=)
    DEFINE_BINOP(NotEqualTo, !=)

    DEFINE_BINFUNC(Isgreater, std::isgreater, isgreater)
    DEFINE_BINFUNC(Isgreaterequal, std::isgreaterequal, isgreaterequal)
    DEFINE_BINFUNC(Isless, std::isless, isless)
    DEFINE_BINFUNC(Islessequal, std::islessequal, islessequal)
    DEFINE_BINFUNC(Islessgreater, std::islessgreater, islessgreater)
    DEFINE_BINFUNC(Isunordered, std::isunordered, isunordered)
#undef DEFINE_BINOP
}// namespace OpFlow

#endif//OPFLOW_COMPARE_HPP
