// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2025 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_OFFSETVECTOR_HPP
#define OPFLOW_OFFSETVECTOR_HPP

#include <format>
#ifndef OPFLOW_INSIDE_MODULE
#include <cmath>
#include <string>
#include <vector>
#endif

OPFLOW_MODULE_EXPORT namespace OpFlow::DS {
    template <typename T>
    struct OffsetVector {
    protected:
        int offset = 0;
        std::vector<T> val;

    public:
        OffsetVector() = default;
        explicit OffsetVector(std::size_t n) : val(n) {}

        const auto& operator[](int i) const { return val[i - offset]; }
        auto& operator[](int i) { return val[i - offset]; }
        const auto& operator()(int i) const { return val[i - offset]; }
        auto& operator()(int i) { return val[i - offset]; }

        void resize(std::size_t n) { val.resize(n); }
        auto size() const { return val.size(); }
        void setOffset(int off) { offset = off; }
        auto getOffset() const { return offset; }
        void setConstant(const T& t) { val.assign(val.size(), t); }

        auto& front() { return val.front(); }
        const auto& front() const { return val.front(); }
        auto& back() { return val.back(); }
        const auto& back() const { return val.back(); }
        auto begin() { return val.begin(); }
        auto end() { return val.end(); }
        auto begin() const { return val.begin(); }
        auto end() const { return val.end(); }

        bool operator==(const OffsetVector& other) const {
            return offset == other.offset && val == other.val;
        }

        auto toString() const {
            std::string ret;
            ret += std::format("offset = {} data = {}", offset, val);
            return ret;
        }

        bool commonRangeEqualTo(const OffsetVector& other, Real tol = 1.e-14) const {
            int commOffset = std::max(offset, other.offset);
            int commEnd = std::min(offset + val.size(), other.offset + other.size());
            if (commOffset >= commEnd) return false;
            auto ret = true;
            if constexpr (std::is_floating_point_v<T>) {
                for (auto i = commOffset + 1; i < commEnd - 1; ++i) {
                    ret &= std::abs(this->operator[](i) - other[i])
                           <= tol * std::max(std::abs(this->operator[](i)), std::abs(other[i]));
                }
            } else {
                for (auto i = commOffset; i < commEnd; ++i) { ret &= this->operator[](i) == other[i]; }
            }
            return ret;
        }
    };
}// namespace OpFlow::DS
#endif//OPFLOW_OFFSETVECTOR_HPP
