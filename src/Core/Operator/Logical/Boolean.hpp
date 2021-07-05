#ifndef OPFLOW_BOOLEAN_HPP
#define OPFLOW_BOOLEAN_HPP

#include "Core/Expr/Expr.hpp"
#include "Core/Expr/Expression.hpp"
#include "Core/Meta.hpp"
#include "Core/Operator/BinOpDefMacros.hpp.in"
#include "Core/Operator/Operator.hpp"
#include "Core/Operator/UniOpDefMacros.hpp.in"

namespace OpFlow {
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
