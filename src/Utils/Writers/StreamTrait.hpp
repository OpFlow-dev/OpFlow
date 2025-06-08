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

#ifndef OPFLOW_STREAMTRAIT_HPP
#define OPFLOW_STREAMTRAIT_HPP

#include "Core/Constants.hpp"
#ifndef OPFLOW_INSIDE_MODULE
#include <type_traits>
#endif

OPFLOW_MODULE_EXPORT namespace OpFlow::Utils {
    namespace internal {
        template <typename T>
        struct StreamTrait;
    }

    template <typename T>
    concept RStreamType = bool(internal::StreamTrait<T>::mode_flag & StreamIn);

    template <typename T>
    concept WStreamType = bool(internal::StreamTrait<T>::mode_flag & StreamOut);
}// namespace OpFlow::Utils
#endif//OPFLOW_STREAMTRAIT_HPP
