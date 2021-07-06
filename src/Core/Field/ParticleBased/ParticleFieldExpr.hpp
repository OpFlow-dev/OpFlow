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

#ifndef OPFLOW_PARTICLEFIELDEXPR_HPP
#define OPFLOW_PARTICLEFIELDEXPR_HPP

#include "Core/Field/FieldExpr.hpp"

namespace OpFlow {
    template <typename Derived>
    struct ParticleFieldExpr : FieldExpr<Derived> {};
}// namespace OpFlow
#endif//OPFLOW_PARTICLEFIELDEXPR_HPP
