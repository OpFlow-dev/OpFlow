//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2021 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

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

#ifndef OPFLOW_CLASSIFY_HPP
#define OPFLOW_CLASSIFY_HPP

namespace OpFlow {

    DEFINE_UNIFUNC(FPclassify, std::fpclassify, fpclassify)
    DEFINE_UNIFUNC(Isfinite, std::isfinite, isfinite)
    DEFINE_UNIFUNC(Isinf, std::isinf, isinf)
    DEFINE_UNIFUNC(Isnan, std::isnan, isnan)
    DEFINE_UNIFUNC(Isnormal, std::isnormal, isnormal)
    DEFINE_UNIFUNC(Signbit, std::signbit, signbit)
}// namespace OpFlow

#endif//OPFLOW_CLASSIFY_HPP
