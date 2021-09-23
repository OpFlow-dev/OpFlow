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

#ifndef OPFLOW_MDINDEX_HPP
#define OPFLOW_MDINDEX_HPP

#include "Core/Interfaces/Serializable.hpp"
#include "Core/Macros.hpp"
#include "Utils/xxHash.hpp"
#include "fmt/format.h"
#include <array>
#include <cassert>
#include <compare>
#include <concepts>
#include <initializer_list>
#include <memory>
#include <vector>

namespace OpFlow::DS {

    template <int d>
    struct MDIndex : SerializableObj<MDIndex<d>> {
    protected:
        std::array<int, d> idx;

    public:
        static constexpr auto dim = d;
        constexpr MDIndex() { idx.fill(0); };
        constexpr explicit MDIndex(int i) { idx.fill(i); }
        constexpr MDIndex(const MDIndex<d>&) noexcept = default;

        template <Meta::WeakIntegral... T>
        constexpr explicit MDIndex(T&&... indexes) noexcept
            : MDIndex(std::array {std::forward<T>(indexes)...}) {}

        constexpr explicit MDIndex(const std::array<int, d>& array) noexcept : idx(array) {}

        constexpr const auto& operator[](int i) const { return idx[i]; }

        // make this const as to forbid modification breaking consistency
        constexpr const auto& get() const { return idx; }
        constexpr auto c_arr() const { return idx.data(); }
        constexpr auto c_arr() { return idx.data(); }

        constexpr auto& operator[](int i) { return this->idx[i]; }

        constexpr void set(const std::array<int, d>& o) { this->idx = o; }

        constexpr void set(const Meta::BracketIndexable auto& o) {
            for (auto i = 0; i < d; ++i) this->idx[i] = o[i];
        }

        constexpr bool operator<(const MDIndex& other) const {
            for (auto i = d - 1; i >= 0; --i) {
                if (idx[i] < other.idx[i]) return true;
                else if (idx[i] > other.idx[i])
                    return false;
            }
            return false;
        }

        constexpr bool operator>(const MDIndex& other) const { return other < *this; }

        constexpr bool operator==(const Meta::BracketIndexable auto& index) const {
            auto ret = true;
            for (auto i = 0; i < d; ++i) ret &= this->idx[i] == index[i];
            return ret;
        }

        constexpr auto operator+(const MDIndex& index) const {
            MDIndex ret;
            for (auto i = 0; i < d; ++i) ret[i] = this->idx[i] + index[i];
            return ret;
        }

        constexpr auto operator-(const MDIndex& index) const {
            MDIndex ret;
            for (auto i = 0; i < d; ++i) ret[i] = this->idx[i] - index[i];
            return ret;
        }

        constexpr auto& operator+=(const MDIndex& index) {
            for (auto i = 0; i < d; ++i) this->idx[i] += index[i];
            return *this;
        }

        constexpr auto& operator-=(const MDIndex& index) {
            for (auto i = 0; i < d; ++i) this->idx[i] -= index[i];
            return *this;
        }

        constexpr auto copy() const { return *this; }

        template <std::size_t dim = 0>
        requires requires {
            dim < d;
        }
        constexpr auto next(int steps = 1) const {
            auto c = copy();
            c.idx[dim] += steps;
            return c;
        }

        template <std::size_t dim = 0>
        requires requires {
            dim < d;
        }
        constexpr auto prev(int steps = 1) const {
            auto c = copy();
            c.idx[dim] -= steps;
            return c;
        }

        constexpr auto flip() const {
            auto c = copy();
            for (auto i = 0; i < d; ++i)
                c[i] = this->idx[d - i - 1];
            return c;
        }

        auto toString() const {
            std::string ret = "{";
            if constexpr (d > 0) ret += fmt::format("{}", idx[0]);
            for (auto i = 1; i < d; ++i) ret += fmt::format(", {}", idx[i]);
            ret += "}";
            return ret;
        }
    };
}// namespace OpFlow::DS

namespace std {

    template <int d>
    struct hash<OpFlow::DS::MDIndex<d>> {
        std::size_t operator()(OpFlow::DS::MDIndex<d> const& i) const noexcept {
            auto idx = i.get();
            return XXHash64::hash(idx.data(), idx.size() * sizeof(int), 0);
        }
    };
}// namespace std
#endif//OPFLOW_MDINDEX_HPP
