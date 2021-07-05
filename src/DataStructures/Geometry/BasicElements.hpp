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

#ifndef OPFLOW_BASICELEMENTS_HPP
#define OPFLOW_BASICELEMENTS_HPP

#include "Core/Macros.hpp"
#include "Core/Meta.hpp"
#include "Utils/Serializer/STDContainers.hpp"
#include <array>
#include <fmt/format.h>
#include <vector>

namespace OpFlow::DS {

    template <Meta::Numerical T, std::size_t d>
    struct Point {
        constexpr static auto dim = d;

        Point() = default;

        Point(const Point &) = default;

        Point(Point &&) noexcept = default;

        template <Meta::Numerical... Ts>
        explicit Point(Ts... idx) : cord {std::forward<Ts>(idx)...} {}

        explicit Point(const std::array<T, d> &arr) : cord(arr) {}

        auto &operator=(const Point &other) {
            cord = other.cord;
            return *this;
        }

        auto toString() const { return Utils::Serializer::serialize(cord); }

        auto &operator[](int i) { return cord[i]; }

        const auto &operator[](int i) const { return cord[i]; }

        auto &operator()(int i) { return cord[i]; }

        const auto &operator()(int i) const { return cord[i]; }

        auto operator<(const Point &other) const {
            for (auto i = 0; i < dim; ++i) {
                if (cord[i] < other.cord[i]) return true;
                else if (cord[i] > other.cord[i])
                    return false;
            }
            return false;
        }

        auto operator==(const Point &other) const {
            for (auto i = 0; i < dim; ++i) {
                if (cord[i] != other.cord[i]) return false;
            }
            return true;
        }

        std::array<T, d> cord;
    };

    template <typename V, Meta::Numerical T, std::size_t d>
    struct PointWithLabel : Point<T, d> {
        V val;
        using Point<T, d>::Point;
        using Point<T, d>::toString;
    };

    template <Meta::Numerical T, std::size_t d>
    struct Segment {
        using PointType = Point<T, d>;
        PointType start, end;
    };

    template <Meta::Numerical T, std::size_t d>
    struct Plane {
        using PointType = Point<T, d>;
        std::array<PointType, d> points;
    };

    template <Meta::Numerical T, std::size_t d>
    struct Box {
        constexpr static auto dim = d;
        std::array<T, d> lo, hi;

        auto toString() const {
            return fmt::format("[{} -> {}]", Point<T, d>(lo).toString(), Point<T, d>(hi).toString());
        }

        auto operator==(const Box &other) const { return lo == other.lo && hi == other.hi; }
    };

    template <typename... Ts>
    auto boxMerge(Ts... boxes) {
        static_assert(sizeof...(Ts) > 0);
        return boxMerge(std::vector {boxes...});
    }

    template <typename T>
    requires Meta::BracketIndexable<T> auto boxMerge(const T &boxes) {
        OP_ASSERT(boxes.size() > 0);
        auto ret = boxes[0];
        constexpr static auto dim = decltype(ret)::dim;
        for (auto i = 1; i < boxes.size(); ++i) {
            for (auto j = 0; j < dim; ++j) {
                ret.lo[j] = std::min(ret.lo[j], boxes[i].lo[j]);
                ret.hi[j] = std::max(ret.hi[j], boxes[i].hi[j]);
            }
        }
        return ret;
    }

    template <Meta::Numerical T, std::size_t d>
    auto pointInBox(const Point<T, d> &p, const Box<T, d> &box) {
        auto ret = true;
        for (auto i = 0; i < d; ++i) { ret &= (box.lo[i] <= p[i] && p[i] <= box.hi[i]); }
        return ret;
    }

    template <Meta::Numerical T, std::size_t d>
    auto boxInBox(const Box<T, d> &a, const Box<T, d> &b) {
        auto ret = true;
        for (auto i = 0; i < d; ++i) { ret &= (b.lo[i] <= a.lo[i] && a.hi[i] <= b.hi[i]); }
        return ret;
    }

    template <Meta::Numerical T, std::size_t d>
    auto boxIntersectBox(const Box<T, d> &a, const Box<T, d> &b) {
        for (auto i = 0; i < d; ++i) {
            auto start = std::max(a.lo[i], b.lo[i]);
            auto end = std::min(a.hi[i], b.hi[i]);
            if (start > end) return false;
        }
        return true;
    }
}// namespace OpFlow::DS
#endif//OPFLOW_BASICELEMENTS_HPP
