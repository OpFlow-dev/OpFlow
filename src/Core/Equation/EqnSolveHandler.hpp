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

#ifndef OPFLOW_EQNSOLVEHANDLER_HPP
#define OPFLOW_EQNSOLVEHANDLER_HPP

namespace OpFlow {
    // abstract interface for all equation solve handler
    struct EqnSolveHandler {
        virtual ~EqnSolveHandler() = default;
        virtual void init() = 0;
        virtual void solve() = 0;
    };
}// namespace OpFlow

#endif//OPFLOW_EQNSOLVEHANDLER_HPP
