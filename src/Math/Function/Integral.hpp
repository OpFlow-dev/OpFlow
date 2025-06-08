//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2023 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_INTEGRAL_HPP
#define OPFLOW_INTEGRAL_HPP

OPFLOW_MODULE_EXPORT namespace OpFlow::Math {

    inline constexpr int round_to_range(int i, int start, int end) {
        int off = i - start;
        int len = end - start;
        return (off % len + len) % len + start;
    }
}// namespace OpFlow::Math

#endif//OPFLOW_INTEGRAL_HPP
