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

#ifndef OPFLOW_LEVELMDINDEX_HPP
#define OPFLOW_LEVELMDINDEX_HPP

#include "Core/Interfaces/Stringifiable.hpp"
#include "DataStructures/Index/MDIndex.hpp"
#include "Math/Function/Numeric.hpp"
#include "Utils/xxHash.hpp"
#ifndef OPFLOW_INSIDE_MODULE
#include <array>
#endif

namespace OpFlow::DS {
    template <std::size_t d>
    struct LevelMDIndex : public StringifiableObj {
        std::array<int, d> idx;
        int l = 0, p = 0;

        constexpr LevelMDIndex() = default;
        constexpr LevelMDIndex(const LevelMDIndex&) = default;

        template <Meta::WeakIntegral... T>
        constexpr explicit LevelMDIndex(int level, int part, T&&... indexes)
            : l(level), p(part), idx(std::array {std::forward<T>(indexes)...}) {}

        constexpr LevelMDIndex(int level, int part, const std::array<int, d>& array)
            : l(level), p(part), idx(array) {}

        constexpr const auto& operator[](int i) const { return idx[i]; }
        constexpr const auto& get() const { return idx; }
        constexpr auto c_arr() { return idx.data(); }
        constexpr auto c_arr() const { return idx.data(); }
        constexpr auto& operator[](int i) { return idx[i]; }
        constexpr void set(const std::array<int, d>& o) { idx = o; }
        constexpr bool operator==(const LevelMDIndex&) const = default;
        constexpr auto operator+(LevelMDIndex index) const { return index += *this; }
        constexpr auto operator-(const LevelMDIndex& index) const {
            auto ret = *this;
            return ret -= index;
        }
        constexpr auto& operator+=(const LevelMDIndex& index) {
            for (auto i = 0; i < d; ++i) idx[i] += index[i];
            return *this;
        }
        constexpr auto& operator-=(const LevelMDIndex& index) {
            for (auto i = 0; i < d; ++i) idx[i] -= index[i];
            return *this;
        }
        template <std::size_t dim>
        constexpr auto next(int steps = 1) const {
            auto c = *this;
            c.idx[dim] += steps;
            return c;
        }
        template <std::size_t dim>
        constexpr auto prev(int steps = 1) const {
            auto c = *this;
            c.idx[dim] -= steps;
            return c;
        }

        auto toLevel(int k, int r) const {
            auto ret = *this;
            for (auto i = 0; i < d; ++i) {
                if (l > k) ret.idx[i] /= Math::int_pow(r, l - k);
                else
                    ret.idx[i] *= Math::int_pow(r, k - l);
            }
            ret.l = k;
            return ret;
        }

        [[nodiscard]] std::string toString(int n, const std::string& prefix) const override {
            std::string ret;
            for (int i = 0; i < n; ++i) ret += prefix;
            ret += "{" + std::format("{}, {}", l, p);
            if constexpr (d > 0) ret += std::format(", {}", this->idx[0]);
            for (auto i = 1; i < d; ++i) ret += std::format(", {}", this->idx[i]);
            ret += "}";
            return ret;
        }

        bool operator<(const LevelMDIndex& other) const {
            if (l < other.l) return true;
            else if (l > other.l)
                return false;
            else {
                for (auto i = d - 1; i >= 0; --i) {
                    if (idx[i] < other[i]) return true;
                    else if (idx[i] > other[i])
                        return false;
                }
            }
            return false;
        }

        bool operator>(const LevelMDIndex& other) const { return other < *this; }

        explicit operator MDIndex<d>() const { return MDIndex<d>(get()); }
    };
}// namespace OpFlow::DS

namespace std {
    template <std::size_t d>
    struct hash<OpFlow::DS::LevelMDIndex<d>> {
        std::size_t operator()(const OpFlow::DS::LevelMDIndex<d>& i) const noexcept {
            auto idx = i.get();
            return XXHash64::hash(idx.data(), idx.size() * sizeof(int), 0);
        }
    };
}// namespace std
#endif//OPFLOW_LEVELMDINDEX_HPP
