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

#ifndef OPFLOW_UNSTRUCTMBFIELDEXPR_HPP
#define OPFLOW_UNSTRUCTMBFIELDEXPR_HPP

#include "Core/Field/MeshBased/MeshBasedFieldExpr.hpp"

OPFLOW_MODULE_EXPORT namespace OpFlow {
    template <typename Derived>
    struct UnStructMBFieldExpr : MeshBasedFieldExpr<Derived> {};
}// namespace OpFlow
#endif//OPFLOW_UNSTRUCTMBFIELDEXPR_HPP
