#ifndef OPFLOW_COMPARE_HPP
#define OPFLOW_COMPARE_HPP

#include "Core/Expr/Expr.hpp"
#include "Core/Expr/Expression.hpp"
#include "Core/Meta.hpp"
#include "Core/Operator/BinOpDefMacros.hpp.in"
#include "Core/Operator/Operator.hpp"

namespace OpFlow {
    DEFINE_BINOP(GreaterThan, >)
    DEFINE_BINOP(GreaterThanOrEqual, >=)
    DEFINE_BINOP(LessThan, <)
    DEFINE_BINOP(LessThanOrEqual, <=)
    DEFINE_BINOP(NotEqualTo, !=)
#undef DEFINE_BINOP
}// namespace OpFlow

#endif//OPFLOW_COMPARE_HPP
