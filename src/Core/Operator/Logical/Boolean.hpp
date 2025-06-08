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

#ifndef OPFLOW_BOOLEAN_HPP
#define OPFLOW_BOOLEAN_HPP

#include "Core/Expr/Expr.hpp"
#include "Core/Expr/Expression.hpp"
#include "Core/Meta.hpp"
#include "Core/Operator/BinOpDefMacros.hpp.in"
#include "Core/Operator/Operator.hpp"
#include "Core/Operator/UniOpDefMacros.hpp.in"

OPFLOW_MODULE_EXPORT namespace OpFlow {
    DEFINE_BINOP(And, &&)
    DEFINE_BINOP(Or, ||)
    DEFINE_UNIOP(Not, !)
    DEFINE_BINOP(BitXor, ^)
    DEFINE_BINOP(BitAnd, &)
    DEFINE_BINOP(BitOr, |)
    //DEFINE_BINOP(BitLShift, <<)
    //DEFINE_BINOP(BitRShift, >>)

#undef DEFINE_BINOP
#undef DEFINE_UNIOP
}// namespace OpFlow

#endif//OPFLOW_BOOLEAN_HPP
