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

#ifndef OPFLOW_FIXEDSIZETENSOR_HPP
#define OPFLOW_FIXEDSIZETENSOR_HPP

#include "Core/Meta.hpp"
#include "DataStructures/Arrays/Tensor/TensorBase.hpp"
#include "DataStructures/Index/MDIndex.hpp"
#ifndef OPFLOW_INSIDE_MODULE
#include <array>
#endif

OPFLOW_MODULE_EXPORT namespace OpFlow::DS {
    namespace internal {
        template <std::size_t N>
        constexpr int accumulate_product(std::array<int, N> arr) {
            if constexpr (N == 0) return 0;
            else {
                int ret = arr[0];
                for (auto i = 1; i < N; ++i) ret *= arr[i];
                return ret;
            }
        }

    }// namespace internal
    template <Meta::Numerical T, auto... n>
    requires(std::integral<decltype(n)> && ...) struct FixedSizeTensor;
    namespace internal {
        template <Meta::Numerical T, auto... n>
        requires(std::integral<decltype(n)>&&...) struct TensorTrait<FixedSizeTensor<T, n...>> {
            using scalar_type = T;
            static constexpr auto dim = sizeof...(n);
        };

        template <typename T>
        struct is_fixed_size_tensor : std::false_type {};

        template <Meta::Numerical T, auto... n>
        requires(std::integral<decltype(n)>&&...) struct is_fixed_size_tensor<FixedSizeTensor<T, n...>>
            : std::true_type {};
    }// namespace internal

    template <typename T>
    concept FixedSizeTensorType = internal::is_fixed_size_tensor<Meta::RealType<T>>::value;

    template <Meta::Numerical T, auto... n>
    requires(std::integral<decltype(n)> && ...) struct FixedSizeTensor : Tensor<FixedSizeTensor<T, n...>> {
        constexpr static auto N = sizeof...(n);
        constexpr FixedSizeTensor() = default;
        constexpr explicit FixedSizeTensor(T val) { _val.fill(val); }
        constexpr explicit FixedSizeTensor(auto... vals) : _val {vals...} {}

        constexpr const auto& operator[](const MDIndex<N>& idx) const {
            int _pos = idx[N - 1];
            for (int i = N - 2; i >= 0; --i) { _pos = _pos * _sizes[i] + idx[i]; }
            return _val[_pos];
        }
        constexpr auto& operator[](const MDIndex<N>& idx) {
            int _pos = idx[N - 1];
            for (int i = N - 2; i >= 0; --i) { _pos = _pos * _sizes[i] + idx[i]; }
            return _val[_pos];
        }
        constexpr const auto& operator[](const int& idx) const { return _val[idx]; }
        constexpr auto& operator[](const int& idx) { return _val[idx]; }
        constexpr const auto& operator()(const MDIndex<N>& idx) const { return operator[](idx); }
        constexpr auto& operator()(const MDIndex<N>& idx) { return operator[](idx); }
        constexpr const auto& operator()(const int& idx) const { return _val[idx]; }
        constexpr auto& operator()(const int& idx) { return _val[idx]; }

        constexpr static auto size() { return _sizes; }
        constexpr static auto total_size() { return _total_size; }
        constexpr static auto size_of(int d) { return _sizes[d]; }
        constexpr static auto max_half_width() {
            if constexpr (N == 0) return 0;
            auto max_dim = _sizes[0];
            for (int i = 1; i < N; ++i) max_dim = std::max(max_dim, _sizes[i]);
            return max_dim / 2;
        }

        constexpr void fill(T val) { _val.fill(val); }

        static constexpr std::array<int, N> _sizes {n...};
        static constexpr int _total_size = internal::accumulate_product(_sizes);
        std::array<T, _total_size> _val;
    };

}// namespace OpFlow::DS
#endif//OPFLOW_FIXEDSIZETENSOR_HPP
