// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2021 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_AMDS_HPP
#define OPFLOW_AMDS_HPP

#include "Core/Expr/Expr.hpp"
#include "Core/Expr/Expression.hpp"
#include "Core/Field/FieldExpr.hpp"
#include "Core/Field/MeshBased/MeshBasedFieldExprTrait.hpp"
#include "Core/Field/MeshBased/SemiStructured/SemiStructuredFieldExprTrait.hpp"
#include "Core/Field/MeshBased/Structured/StructuredFieldExprTrait.hpp"
#include "Core/Meta.hpp"
#include "Core/Operator/BinOpDefMacros.hpp.in"
#include "Core/Operator/Operator.hpp"
#include "Core/Operator/UniOpDefMacros.hpp.in"
#include <cmath>
#include <type_traits>
#include <utility>

namespace OpFlow {

    DEFINE_BINOP(Add, +)
    DEFINE_BINOP(Mul, *)
    DEFINE_BINOP(Sub, -)
    DEFINE_BINOP(Div, /)
    DEFINE_BINOP(Mod, %)
    DEFINE_BINFUNC(Pow, std::pow, pow)
    DEFINE_BINFUNC(FMod, std::fmod, fmod)
    DEFINE_BINFUNC(Remainder, std::remainder, remainder)
    DEFINE_BINFUNC(FDim, std::fdim, fdim)
    DEFINE_BINFUNC(Hypot, std::hypot, hypot)
    DEFINE_BINFUNC(ATan2, std::atan2, atan2)
    DEFINE_BINFUNC(Ldexp, std::ldexp, ldexp)
    DEFINE_BINFUNC(Scalbn, std::scalbn, scalbn)
    DEFINE_BINFUNC(Scalbln, std::scalbln, scalbln)
    DEFINE_BINFUNC(Nextafter, std::nextafter, nextafter)
    DEFINE_BINFUNC(Nexttoward, std::nexttoward, nexttoward)
    DEFINE_BINFUNC(Copysing, std::copysign, copysign)

    DEFINE_UNIOP(Neg, -)
    DEFINE_UNIOP(Pos, +)
    DEFINE_UNIFUNC(Abs, std::abs, abs)
    DEFINE_UNIFUNC(Exp, std::exp, exp)
    DEFINE_UNIFUNC(Exp2, std::exp2, exp2)
    DEFINE_UNIFUNC(Expm1, std::expm1, expm1)
    DEFINE_UNIFUNC(Log, std::log, log)
    DEFINE_UNIFUNC(Log10, std::log10, log10)
    DEFINE_UNIFUNC(Log2, std::log2, log2)
    DEFINE_UNIFUNC(Log1p, std::log1p, log1p)
    DEFINE_UNIFUNC(Sqrt, std::sqrt, sqrt)
    DEFINE_UNIFUNC(Cbrt, std::cbrt, cbrt)
    DEFINE_UNIFUNC(Sin, std::sin, sin)
    DEFINE_UNIFUNC(Cos, std::cos, cos)
    DEFINE_UNIFUNC(Tan, std::tan, tan)
    DEFINE_UNIFUNC(ASin, std::asin, asin)
    DEFINE_UNIFUNC(ACos, std::acos, acos)
    DEFINE_UNIFUNC(ATan, std::atan, atan)
    DEFINE_UNIFUNC(Sinh, std::sinh, sinh)
    DEFINE_UNIFUNC(Cosh, std::cosh, cosh)
    DEFINE_UNIFUNC(Tanh, std::tanh, tanh)
    DEFINE_UNIFUNC(ASinh, std::asinh, asinh)
    DEFINE_UNIFUNC(ACosh, std::acosh, acosh)
    DEFINE_UNIFUNC(ATanh, std::atanh, atanh)
    DEFINE_UNIFUNC(Erf, std::erf, erf)
    DEFINE_UNIFUNC(Erfc, std::erfc, erfc)
    DEFINE_UNIFUNC(TGamma, std::tgamma, tgamma)
    DEFINE_UNIFUNC(LGamma, std::lgamma, lgamma)
    DEFINE_UNIFUNC(Ceil, std::ceil, ceil)
    DEFINE_UNIFUNC(Floor, std::floor, floor)
    DEFINE_UNIFUNC(Trunc, std::trunc, trunc)
    DEFINE_UNIFUNC(Round, std::round, round)
    DEFINE_UNIFUNC(LRound, std::lround, lround)
    DEFINE_UNIFUNC(LLRound, std::llround, llround)
    DEFINE_UNIFUNC(NearbyInt, std::nearbyint, nearbyint)
    DEFINE_UNIFUNC(Rint, std::rint, rint)
    DEFINE_UNIFUNC(LRint, std::lrint, lrint)
    DEFINE_UNIFUNC(LLRint, std::llrint, llrint)
    DEFINE_UNIFUNC(ILogb, std::ilogb, ilogb)
    DEFINE_UNIFUNC(Logb, std::logb, logb)

#undef DEFINE_BINOP
#undef DEFINE_BINFUNC
#undef DEFINE_UNIOP
#undef DEFINE_UNIFUNC
}// namespace OpFlow
#endif//OPFLOW_AMDS_HPP
