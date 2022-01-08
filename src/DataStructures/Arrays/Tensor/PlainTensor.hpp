// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2022 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_PLAINTENSOR_HPP
#define OPFLOW_PLAINTENSOR_HPP

#include "Core/Loops/RangeFor.hpp"
#include "Core/Macros.hpp"
#include "Core/Meta.hpp"
#include "DataStructures/Index/MDIndex.hpp"
#include "DataStructures/Index/RangedIndex.hpp"
#include "TensorBase.hpp"
#include "Utils/Allocator/AlignedAllocator.hpp"
#include "Utils/Allocator/StaticAllocator.hpp"
#include <array>
#include <concepts>
#include <memory>

namespace OpFlow::DS {
    template <typename ScalarType, int d, typename Allocator>
    requires Utils::StaticAllocatorType<ScalarType, Allocator> struct PlainTensor;

    namespace internal {
        template <typename ScalarType, int d, typename Allocator>
        requires Utils::StaticAllocatorType<ScalarType, Allocator> struct TensorTrait<
                PlainTensor<ScalarType, d, Allocator>> {
            using scalar_type = ScalarType;
            static constexpr auto dim = d;
            using allocator_type = Allocator;
            template <typename T>
            using other_type = PlainTensor<
                    T, d, typename Utils::internal::AllocatorTrait<Allocator>::template other_type<T>>;
            template <typename T>
            using other_alloc_type = PlainTensor<ScalarType, d, T>;
            template <int t>
            using other_dim_type = PlainTensor<ScalarType, t, Allocator>;
        };
    }// namespace internal

    template <typename ScalarType, int d, typename Allocator = Utils::AlignedAllocator<ScalarType>>
    requires Utils::StaticAllocatorType<ScalarType, Allocator> struct PlainTensor
        : public Tensor<PlainTensor<ScalarType, d, Allocator>> {
    private:
        ScalarType* data = nullptr;

    public:
        std::array<int, d> dims;
        long long total_size = 1, allocated_size = 0;

        using Scalar = ScalarType;
        using value_type = Scalar;// mocking std::vector

        PlainTensor() { dims.fill(0); }
        ~PlainTensor() { Allocator::deallocate(data, allocated_size); }

        explicit PlainTensor(std::integral auto size, std::integral auto... sizes) {
            reShape(size, sizes...);
        }

        explicit PlainTensor(const std::array<int, d>& size) { reShape(size); }

        auto& reShape(std::integral auto size, std::integral auto... sizes) {
            return reShape(std::array {size, sizes...});
        }

        auto& reShape(const std::array<int, d>& sizes) {
            if (data) Allocator::deallocate(data, allocated_size);
            dims = sizes;
            total_size = 1;
            for (auto i = 0; i < d; ++i) { total_size *= dims[i]; }
            data = Allocator::allocate(total_size);
            allocated_size = total_size;
            return *this;
        }

        auto& resize(std::integral auto size, std::integral auto... sizes) {
            return resize(std::array<int, d> {(int) size, (int) sizes...});
        }

        auto& resize(const std::array<int, d>& sizes) {
            if (DS::Range<d> {dims}.covers(DS::Range<d> {sizes})) {
                dims = sizes;
                total_size = 1;
                for (auto i = 0; i < d; ++i) total_size *= sizes[i];
            } else {
                ScalarType* new_data;
                total_size = 1;
                for (auto i = 0; i < d; ++i) total_size *= sizes[i];
                new_data = Allocator::allocate(total_size);
                if (data) {
                    rangeFor(DS::commonRange(DS::Range<d> {dims}, DS::Range<d> {sizes}),
                             [&](auto&& k) { new_data[getOffset(k)] = data[getOffset(k)]; });
                    Allocator::deallocate(data, allocated_size);
                }
                allocated_size = total_size;
                data = new_data;
                dims = sizes;
            }
            return *this;
        }

        template <typename OtherScalar>
        explicit PlainTensor(const PlainTensor<OtherScalar, d>& other)
            : dims(other.dims), total_size(other.total_size) {
            if (other.raw() == nullptr) return;
            data = Allocator::allocate(total_size);
            allocated_size = total_size;
            // deep copy of data, assuming OtherScalar can be converted to Scalar
            std::copy(other.raw(), other.raw() + total_size, this->raw());
        }

        PlainTensor(const PlainTensor& other)// copy the above impl because we can't call a templated ctor
            : dims(other.dims), total_size(other.total_size) {
            if (other.raw() == nullptr) return;
            data = Allocator::allocate(total_size);
            allocated_size = total_size;
            // deep copy of data, assuming OtherScalar can be converted to Scalar
            std::copy(other.raw(), other.raw() + total_size, this->raw());
        }

        PlainTensor(PlainTensor&& other) noexcept
            : dims(std::move(other.dims)), total_size(other.total_size) {
            data = other.data;
            other.data = nullptr;
        }

        // operator= is simply treated as assignment
        template <typename OtherScalar>
        requires(!std::is_same_v<Scalar, OtherScalar>) auto&
        operator=(const PlainTensor<OtherScalar, d>& other) {
            OP_ASSERT(other.raw())// assign to an empty tensor is an error
            if (!data) {
                reShape(other);
            } else {
                assert(total_size == other.total_size);
            }
            std::copy(other.raw(), other.raw() + total_size, data);
            return *this;
        }

        auto& operator=(const PlainTensor& other) {
            if (this == &other) return *this;
            OP_ASSERT(other.raw())
            if (data == nullptr) {
                reShape(other);
            } else {
                assert(total_size == other.total_size);
            }
            std::copy(other.raw(), other.raw() + total_size, data);
            return *this;
        }

        auto& operator=(PlainTensor&& other) noexcept {
            dims = std::move(other.dims);
            total_size = other.total_size;
            data = other.data;
            other.data = nullptr;
            return *this;
        }

        template <typename OtherScalar>
        void reShape(const PlainTensor<OtherScalar, d>& other) {
            if (data) Allocator::deallocate(data, allocated_size);
            dims = other.dims;
            total_size = other.total_size;
            data = Allocator::allocate(total_size);
            allocated_size = total_size;
        }

        auto operator==(const PlainTensor& other) const { return raw() == other.raw(); }

        void setConstant(Scalar t) { std::fill(data, data + total_size, t); }

        void setZero() { setConstant(Scalar(0)); }

        auto begin() { return data; }
        auto begin() const { return data; }

        auto end() { return data + total_size; }
        auto end() const { return data + total_size; }

        auto& front() { return *data; }
        const auto& front() const { return *data; }
        auto& back() { return data[total_size - 1]; }
        const auto& back() const { return data[total_size - 1]; }

        auto size() const { return total_size; }
        auto getDims() const { return dims; }

    private:
        template <Meta::BracketIndexable Idx>
        auto getOffset(const Idx& index) const {
            auto pos = 0;
            for (auto i = d - 1; i >= 1; --i) {
                pos += index[i];
                pos *= dims[i - 1];
            }
            pos += index[0];
            return pos;
        }

        auto getOffset(int index) const { return index; }

        template <typename... I>
                requires(std::integral<Meta::RealType<I>>&&...)
                && (sizeof...(I) > 1) auto getOffset(I&&... i) const {
            return getOffset(DS::MDIndex<d> {i...});
        }

    public:
        // operator() version of indexing (lsj prefers this :)
        template <typename... T>
        const auto& operator()(T&&... index) const {
            return data[getOffset(std::forward<T>(index)...)];
        }

        template <typename... T>
        auto& operator()(T&&... index) {
            return data[getOffset(std::forward<T>(index)...)];
        }

        // operator[] version of indexing
        template <typename T>
        const auto& operator[](T&& index) const {
            return data[getOffset(std::forward<T>(index))];
        }

        template <typename T>
        auto& operator[](T&& index) {
            return data[getOffset(std::forward<T>(index))];
        }

        // linear index getter
        const auto& get(const std::integral auto& idx) const { return data[idx]; }

        auto& get(const std::integral auto& idx) { return data[idx]; }

        auto raw() { return data; }

        auto raw() const { return data; }

        void swap(PlainTensor& other) {
            OP_ASSERT(dims == other.dims);
            std::swap(data, other.data);
        }
    };

    template <typename ScalarType>
    struct PlainTensor<ScalarType, 0> : public Tensor<PlainTensor<ScalarType, 0>> {
    private:
        ScalarType val;

    public:
        std::array<int, 0> dims {};
        long long total_size = 1;

        using Scalar = ScalarType;

        PlainTensor() = default;

        auto& reShape(auto&&) { return *this; }

        template <typename OtherScalar>
        explicit PlainTensor(const PlainTensor<OtherScalar, 0>& other) : val(*other.raw()) {}

        PlainTensor(const PlainTensor& other) = default;
        PlainTensor(PlainTensor&& other) noexcept = default;

        template <typename OtherScalar>
        auto& operator=(const PlainTensor<OtherScalar, 0>& other) {
            this->val = *other.raw();
            return *this;
        }

        auto& operator=(const PlainTensor& other) { return this->template operator=<Scalar>(other); }

        auto& operator==(const PlainTensor& other) const { return this->val == other.val; }

        void setConstant(Scalar t) { val = t; }

        void setZero() { val = 0; }

        auto begin() { return &val; }
        auto begin() const { return &val; }

        auto end() { return &val + 1; }
        auto end() const { return &val + 1; }

        auto size() const { return 1; }
        auto shape() const { return dims; }

        const auto& operator()(auto&&...) const { return val; }
        auto& operator()(auto&&...) { return val; }
        const auto& operator[](auto&&) const { return val; }
        auto& operator[](auto&&) { return val; }
        const auto& get(const std::integral auto&) const { return val; }
        auto& get(const std::integral auto&) { return val; }
        auto raw() { return &val; }
        auto raw() const { return &val; }
        void swap(PlainTensor& other) {
            auto t = val;
            val = other.val;
            other.val = t;
        }
    };
}// namespace OpFlow::DS

#endif//OPFLOW_PLAINTENSOR_HPP
