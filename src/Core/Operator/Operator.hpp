// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2026 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_OPERATOR_HPP
#define OPFLOW_OPERATOR_HPP

#include "Core/Meta.hpp"

OPFLOW_MODULE_EXPORT namespace OpFlow {
    template <typename Op>
    struct ArgChecker;

    template <typename Op, typename... Args>
    struct ResultType;
}// namespace OpFlow
#endif//OPFLOW_OPERATOR_HPP
