//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2026 by the OpFlow developers
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

OPFLOW_MODULE_EXPORT namespace OpFlow {
    struct EqnSolveState {
        int niter = 0;
        double relerr = 0, abserr = 0;
        EqnSolveState() = default;
        explicit EqnSolveState(int n) : niter(n) {}
        explicit EqnSolveState(double e) : relerr(e) {}
        EqnSolveState(int n, double e) : niter(n), relerr(e) {}
        EqnSolveState(int n, double e, double a) : niter(n), relerr(e), abserr(a) {}
    };

    // abstract interface for all equation solve handler
    struct EqnSolveHandler {
        virtual ~EqnSolveHandler() = default;
        virtual void init() = 0;
        virtual EqnSolveState solve() = 0;
        virtual void generateAb() {};
    };
}// namespace OpFlow

#endif//OPFLOW_EQNSOLVEHANDLER_HPP
