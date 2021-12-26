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

#ifndef OPFLOW_COLOREDINDEX_HPP
#define OPFLOW_COLOREDINDEX_HPP

#include "Core/Interfaces/Serializable.hpp"
#include "DataStructures/Index/LevelMDIndex.hpp"
#include "DataStructures/Index/MDIndex.hpp"
#include "fmt/format.h"
#include <string>

namespace OpFlow::DS {
    template <typename T>
    struct ColoredIndex;

    template <std::size_t d>
    struct ColoredIndex<MDIndex<d>> {
    protected:
        std::array<int, d> idx;
        int color = 0;

    public:
        static constexpr auto dim = d;
        constexpr ColoredIndex() { idx.fill(0); };
        constexpr explicit ColoredIndex(const MDIndex<d>& i, int c = 0) : idx(i.get()), color(c) {};

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

        constexpr bool operator<(const MDIndex<d>& other) const {
            for (auto i = d - 1; i >= 0; --i) {
                if (idx[i] < other.idx[i]) return true;
                else if (idx[i] > other.idx[i])
                    return false;
            }
            return false;
        }

        constexpr bool operator>(const MDIndex<d>& other) const { return other < *this; }

        constexpr bool operator==(const Meta::BracketIndexable auto& index) const {
            auto ret = true;
            for (auto i = 0; i < d; ++i) ret &= this->idx[i] == index[i];
            return ret;
        }

        constexpr auto operator+(const MDIndex<d>& index) const {
            auto ret = *this;
            ret += index;
            return ret;
        }

        constexpr auto operator-(const MDIndex<d>& index) const {
            auto ret = *this;
            ret -= index;
            return ret;
        }

        constexpr auto& operator+=(const MDIndex<d>& index) {
            for (auto i = 0; i < d; ++i) this->idx[i] += index[i];
            return *this;
        }

        constexpr auto& operator+=(const ColoredIndex& index) {
            OP_ASSERT_MSG(color == index.color, "Indexes' colors mismatch: {} != {}", color, index.color);
            for (auto i = 0; i < d; ++i) this->idx[i] += index[i];
            return *this;
        }

        constexpr auto& operator-=(const MDIndex<d>& index) {
            for (auto i = 0; i < d; ++i) this->idx[i] -= index[i];
            return *this;
        }

        constexpr auto& operator-=(const ColoredIndex& index) {
            OP_ASSERT_MSG(color == index.color, "Indexes' colors mismatch: {} != {}", color, index.color);
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
            for (auto i = 0; i < d; ++i) c[i] = this->idx[d - i - 1];
            return c;
        }

        auto toString() const {
            std::string ret = "{";
            if constexpr (d > 0) ret += fmt::format("{}", idx[0]);
            for (auto i = 1; i < d; ++i) ret += fmt::format(", {}", idx[i]);
            ret += fmt::format(", c = {}", color);
            ret += "}";
            return ret;
        }
    };
}// namespace OpFlow::DS

#endif//OPFLOW_COLOREDINDEX_HPP
