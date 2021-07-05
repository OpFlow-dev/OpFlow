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

#ifndef OPFLOW_HELPERS_HPP
#define OPFLOW_HELPERS_HPP

#include "Core/Meta.hpp"
#include "fmt/format.h"
#include <string>

namespace OpFlow {
    inline std::string dimString(int d) {
        switch (d) {
            case 0:
                return "x";
            case 1:
                return "y";
            case 2:
                return "z";
            default:
                return fmt::format("x_{}", d);
        }
    }

    template <typename Kernel, typename DecayedKernel>
    requires(DecayedKernel::bc_width < Kernel::bc_width) struct ComposedKernelImpl {
        constexpr static auto bc_width = Kernel::bc_width;

        OPFLOW_STRONG_INLINE static auto couldSafeEval(auto&&... args) {
            return DecayedKernel::couldSafeEval(std::forward<decltype(args)>(args)...)
                   || Kernel::couldSafeEval(std::forward<decltype(args)>(args)...);// this is not necessary
        }

        template <ExprType E, typename I>
        OPFLOW_STRONG_INLINE static auto eval_safe(E&& e, I&& i) {
            if (Kernel::couldSafeEval(std::forward<E>(e), std::forward<I>(i))) {
                return Kernel::eval_safe(std::forward<E>(e), std::forward<I>(i));
            } else {
                return DecayedKernel::eval_safe(std::forward<E>(e), std::forward<I>(i));
            }
        }

        template <ExprType E, typename I>
        OPFLOW_STRONG_INLINE static auto eval(E&& e, I&& i) {
            return Kernel::eval(std::forward<E>(e), std::forward<I>(i));
        }
    };

    template <typename Kernel, typename DecayedKernel, typename... DecayedKernels>
    struct ComposedKernel : ComposedKernel<ComposedKernelImpl<Kernel, DecayedKernel>, DecayedKernels...> {};

    template <typename Kernel, typename DecayedKernel>
    struct ComposedKernel<Kernel, DecayedKernel> : ComposedKernelImpl<Kernel, DecayedKernel> {};
}// namespace OpFlow
#endif//OPFLOW_HELPERS_HPP
