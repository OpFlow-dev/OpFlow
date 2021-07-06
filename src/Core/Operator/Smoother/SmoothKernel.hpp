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

#ifndef OPFLOW_SMOOTHKERNEL_HPP
#define OPFLOW_SMOOTHKERNEL_HPP

#include "Core/Meta.hpp"
#include "StencilKernelTrait.hpp"
#include <array>

namespace OpFlow::Core::Operator {

    template <typename Derived>
    struct SmoothKernelBase {
        template <typename U, typename R, typename... Index>
        OPFLOW_STRONG_INLINE
                typename Field::FieldTrait<U>::Scalar static smoothAt(const U &u, const R &r, long i, long j,
                                                                      Index... k) {
            return Derived::smoothAt(u, r, i, j, k...);
        }
    };

    struct IdenticalSmoothKernel : public SmoothKernelBase<IdenticalSmoothKernel> {
        template <typename U, typename R, typename... Index>
        OPFLOW_STRONG_INLINE
                typename Field::FieldTrait<U>::Scalar static smoothAt(const U &u, const R &r, long i, long j,
                                                                      Index... k) {
            return u(i, j, k...);
        }
    };

    template <typename StencilKernel>
    struct StencilSmoothKernel : public SmoothKernelBase<StencilSmoothKernel<StencilKernel>> {
        template <typename U, typename R, typename... Index>
        OPFLOW_STRONG_INLINE
                typename Field::FieldTrait<U>::Scalar static smoothAt(const U &u, const R &r, long i, long j,
                                                                      Index... k) {
            auto width = StencilKernelTrait<StencilKernel>::width::value;
            if constexpr (sizeof...(k) == 0) {
                using dtype = typename Field::FieldTrait<U>::Scalar;
                static_assert(StencilKernelTrait<StencilKernel>::dim::value == 2);
                std::array<dtype, std::tuple_size_v<decltype(StencilKernel::w)>> ut, rt;
                for (int l = 0; l < ut.size(); ++l) {
                    auto bit0 = l % (width + 1) - width / 2;
                    auto bit1 = l / (width + 1) - width / 2;
                    ut[l] = u(i + bit0, j + bit1);
                    rt[l] = r(i + bit0, j + bit1);
                }
                auto result = dtype(0.), sumOfWeight = dtype(0.);
                for (int l = 0; l < ut.size(); ++l) {
                    result += ut[l] * rt[l] * StencilKernel::w[l];
                    sumOfWeight += rt[l] * StencilKernel::w[l];
                }
                return result / sumOfWeight;
            } else if constexpr (sizeof...(k) == 1) {
                using dtype = typename Field::FieldTrait<U>::Scalar;
                static_assert(StencilKernelTrait<StencilKernel>::dim::value == 3);
                std::array<dtype, std::tuple_size_v<decltype(StencilKernel::w)>> ut, rt;
                for (int l = 0; l < ut.size(); ++l) {
                    auto bit0 = l % (width + 1) - width / 2;
                    auto bit1 = l % ((width + 1) * (width + 1)) - width / 2;
                    auto bit2 = l / ((width + 1) * (width + 1)) - width / 2;
                    ut[l] = u(i + bit0, j + bit1, (k + bit2)...);
                    rt[l] = r(i + bit0, j + bit1, (k + bit2)...);
                }
                auto result = dtype(0.), sumOfWeight = dtype(0.);
                for (int l = 0; l < ut.size(); ++l) {
                    result += ut[l] * rt[l] * StencilKernel::w[l];
                    sumOfWeight += rt[l] * StencilKernel::w[l];
                }
                return result / sumOfWeight;
            }
        }
    };
}// namespace OpFlow::Core::Operator
#endif//OPFLOW_SMOOTHKERNEL_HPP
