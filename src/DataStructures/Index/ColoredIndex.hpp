//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2026 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_COLOREDINDEX_HPP
#define OPFLOW_COLOREDINDEX_HPP

#include "Core/Interfaces/Stringifiable.hpp"
#include "DataStructures/Index/LevelMDIndex.hpp"
#include "DataStructures/Index/MDIndex.hpp"
#include <format>
#ifndef OPFLOW_INSIDE_MODULE
#include <string>
#endif

OPFLOW_MODULE_EXPORT namespace OpFlow::DS {
    template <typename T>
    struct ColoredIndex;

    template <std::size_t d>
    struct ColoredIndex<MDIndex<d>> : public StringifiableObj {
    protected:
        std::array<int, d> idx;

    public:
        int color = 0;
        static constexpr auto dim = d;
        constexpr ColoredIndex() { idx.fill(0); };
        constexpr explicit ColoredIndex(const MDIndex<d>& i, int c = 0) : idx(i.get()), color(c) {};

        constexpr const auto& operator[](int i) const { return idx[i]; }

        // make this const as to forbid modification breaking consistency
        constexpr const auto& get() const { return idx; }
        constexpr auto c_arr() const { return idx.data(); }
        constexpr auto c_arr() { return idx.data(); }
        constexpr explicit operator MDIndex<d>() const { return MDIndex<d> {idx}; }

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

        constexpr bool operator==(const ColoredIndex& other) const {
            auto ret = color == other.color;
            ret &= this->idx == other.idx;
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

        [[nodiscard]] std::string toString(int n, const std::string& prefix) const override {
            std::string ret;
            for (auto i = 0; i < n; ++i) ret += prefix;
            ret += "{";
            if constexpr (d > 0) ret += std::format("{}", idx[0]);
            for (auto i = 1; i < d; ++i) ret += std::format(", {}", idx[i]);
            ret += std::format(", c = {}", color);
            ret += "}";
            return ret;
        }
    };

    template <std::size_t d>
    struct ColoredIndex<LevelMDIndex<d>> : public StringifiableObj {
        std::array<int, d> idx;
        int l = 0, p = 0, color = 0;

        constexpr ColoredIndex() = default;
        constexpr ColoredIndex(const ColoredIndex&) = default;

        constexpr explicit ColoredIndex(const LevelMDIndex<d>& index, int color = 0)
            : idx(index.get()), l(index.l), p(index.p), color(color) {}

        constexpr const auto& operator[](int i) const { return idx[i]; }
        constexpr const auto& get() const { return idx; }
        constexpr auto c_arr() { return idx.data(); }
        constexpr auto c_arr() const { return idx.data(); }
        constexpr auto& operator[](int i) { return idx[i]; }
        constexpr void set(const std::array<int, d>& o) { idx = o; }
        constexpr bool operator==(const LevelMDIndex<d>& index) const {
            return idx == index.idx && l == index.l && p == index.p;
        }
        constexpr bool operator==(const ColoredIndex& other) const {
            return idx == other.idx && l == other.l && p == other.p && color == other.color;
        }
        constexpr auto operator+(const LevelMDIndex<d>& index) const {
            auto ret = *this;
            ret += index;
            return ret;
        }
        constexpr auto operator-(const LevelMDIndex<d>& index) const {
            auto ret = *this;
            ret -= index;
            return ret;
        }
        constexpr auto operator+(const ColoredIndex& index) const {
            auto ret = *this;
            ret += index;
            return ret;
        }
        constexpr auto operator-(const ColoredIndex& index) const {
            auto ret = *this;
            ret -= index;
            return ret;
        }

        constexpr auto& operator+=(const LevelMDIndex<d>& index) {
            // note: we don't check l & p here because +=/-= is usually append to offset whose l&p is not used
            for (auto i = 0; i < d; ++i) idx[i] += index[i];
            return *this;
        }
        constexpr auto& operator-=(const LevelMDIndex<d>& index) {
            for (auto i = 0; i < d; ++i) idx[i] -= index[i];
            return *this;
        }
        constexpr auto& operator+=(const ColoredIndex& index) {
            OP_ASSERT_MSG(color == index.color, "Indexes' colors mismatch: {} != {}", color, index.color);
            // note: we don't check l & p here because +=/-= is usually append to offset whose l&p is not used
            for (auto i = 0; i < d; ++i) idx[i] += index[i];
            return *this;
        }
        constexpr auto& operator-=(const ColoredIndex& index) {
            OP_ASSERT_MSG(color == index.color, "Indexes' colors mismatch: {} != {}", color, index.color);
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
            for (auto i = 0; i < n; ++i) ret += prefix;
            ret += "{" + std::format("{}, {}", l, p);
            if constexpr (d > 0) ret += std::format(", {}", this->idx[0]);
            for (auto i = 1; i < d; ++i) ret += std::format(", {}", this->idx[i]);
            ret += "}";
            return ret;
        }

        bool operator<(const LevelMDIndex<d>& other) const {
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

        bool operator<(const ColoredIndex& other) const { return this->operator<(LevelMDIndex<d> {other}); }

        bool operator>(const LevelMDIndex<d>& other) const { return other < LevelMDIndex<d>(*this); }
        bool operator>(const ColoredIndex& other) const { return this->operator>(LevelMDIndex<d> {other}); }

        explicit operator MDIndex<d>() const { return MDIndex<d>(get()); }
        explicit operator LevelMDIndex<d>() const { return LevelMDIndex<d> {get(), l, p}; }
    };
}// namespace OpFlow::DS

namespace std {
    template <std::size_t d>
    struct hash<OpFlow::DS::ColoredIndex<OpFlow::DS::MDIndex<d>>> {
        std::size_t operator()(OpFlow::DS::ColoredIndex<OpFlow::DS::MDIndex<d>> const& i) const noexcept {
            auto idx = i.get();
            return XXHash64::hash(idx.data(), idx.size() * sizeof(int), 0);
        }
    };

    template <std::size_t d>
    struct hash<OpFlow::DS::ColoredIndex<OpFlow::DS::LevelMDIndex<d>>> {
        std::size_t operator()(const OpFlow::DS::ColoredIndex<OpFlow::DS::LevelMDIndex<d>>& i) const
                noexcept {
            auto idx = i.get();
            return XXHash64::hash(idx.data(), idx.size() * sizeof(int), 0);
        }
    };
}// namespace std

#endif//OPFLOW_COLOREDINDEX_HPP
