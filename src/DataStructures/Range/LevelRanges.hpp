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

#ifndef OPFLOW_LEVELRANGES_HPP
#define OPFLOW_LEVELRANGES_HPP

#include "Core/Macros.hpp"
#include "DataStructures/Index/LevelMDIndex.hpp"
#include "Ranges.hpp"
#ifndef OPFLOW_INSIDE_MODULE
#include <array>
#include <concepts>
#endif

namespace OpFlow::DS {
    template <std::size_t dim>
    struct LevelRangedIndex;

    template <std::size_t d>
    struct LevelRange {
        constexpr static int dim = d;
        using index_type = LevelRangedIndex<dim>;
        using base_index_type = LevelMDIndex<dim>;
        std::array<int, d> start, end, stride;
        int level = 0, part = 0;

    private:
        mutable std::array<int, d> pace;

    public:
        constexpr static auto EmptyRange() {
            LevelRange ret;
            ret.start.fill(0);
            ret.end.fill(0);
            ret.stride.fill(1);
            ret.pace.fill(0);
            ret.level = 0;
            ret.part = 0;
            return ret;
        }

        constexpr LevelRange() { stride.fill(1); }
        constexpr explicit LevelRange(const std::array<int, d>& end) noexcept : end(end), pace(end) {
            start.fill(0);
            stride.fill(1);
        }
        constexpr LevelRange(const std::array<int, d>& start, const std::array<int, d>& end) noexcept
            : start(start), end(end) {
            stride.fill(1);
            reValidPace();
        }
        constexpr LevelRange(const std::array<int, d>& start, const std::array<int, d>& end,
                             const std::array<int, d>& stride) noexcept
            : start(start), end(end), stride(stride) {
            reValidPace();
        }
        constexpr explicit LevelRange(const Range<dim>& range)
            : start(range.start), end(range.end), stride(range.stride) {
            reValidPace();
        }
        constexpr LevelRange(int l, int p, const Range<dim>& range)
            : start(range.start), end(range.end), stride(range.stride), level(l), part(p) {
            reValidPace();
        }
        constexpr explicit operator Range<dim>() const { return Range<dim>(start, end, stride); }
        auto toRange() const { return Range<dim>(start, end, stride); }
        constexpr void reValidPace() const {
            for (auto i = 0; i < dim; ++i) pace[i] = (end[i] - start[i]) / stride[i];
        }
        constexpr auto operator==(const LevelRange& other) const {
            return start == other.start && end == other.end && stride == other.stride && level == other.level
                   && part == other.part;
        }
        constexpr auto check() const {
            bool ret = true;
            ret &= (d == start.size()) && (d == end.size()) && (d == stride.size());
            for (auto i = 0; i < d; ++i) {
                ret &= (end[i] >= start[i]);
                ret &= (stride[i] >= 1);
            }
            return ret;
        }
        constexpr auto count() const {
            // make sure the pace is valid
            reValidPace();
            // make sure the range is not empty
            for (auto i = 0; i < d; ++i)
                if (pace[i] < 0) return 0;
            auto ret = 1;
            for (auto i = 0; i < d; ++i) { ret *= pace[i]; }
            return ret;
        }

        constexpr const auto& getPace() const { return pace; }

        /// \brief Get the sliced range (1 layer thick) along dim \p k at position \p pos
        /// \param k normal dimension of the slice
        /// \param pos position of the slice
        constexpr auto slice(int k, int pos) const {
            assert(k < d);
            auto ret = *this;
            ret.start[k] = pos;
            ret.end[k] = pos + 1;
            ret.pace[k] = 1;
            return ret;
        }

        /// \brief Get the sliced range along dim \p k from \p pos_start to \p pos_end
        /// \param k Normal dimension of the slice
        /// \param pos_start Position of the start
        /// \param pos_end Position of the end
        /// \return The sliced range
        constexpr auto slice(int k, int pos_start, int pos_end) const {
            assert(k < d);
            auto ret = *this;
            ret.start[k] = pos_start;
            ret.end[k] = pos_end;
            ret.pace[k] = pos_end - pos_start;
            return ret;
        }

        auto toString() const {
            std::string ret;
            ret = std::format("{{l = {}, p = {}, {} - {} by {}}}", level, part, start, end, stride);
            return ret;
        }

        constexpr auto getExtends() const {
            reValidPace();
            std::array<int, d> ret;
            for (auto i = 0; i < d; ++i) { ret[i] = pace[i]; }
            return ret;
        }

        constexpr auto getOffset() const {
            std::array<int, d> ret;
            for (auto i = 0; i < d; ++i) ret[i] = start[i];
            return ret;
        }

        auto getBCRanges(int width) const {
            std::vector<LevelRange> ret;
            for (auto i = 0; i < d; ++i) {
                ret.push_back(*this);
                ret.back().end[i] = ret.back().start[i] + width;
                ret.back().pace[i] = width;
                ret.push_back(*this);
                ret.back().start[i] = ret.back().end[i] - width;
                ret.back().pace[i] = width;
            }
            return ret;
        }

        auto getInnerRange(int width) const {
            auto ret = *this;
            for (auto i = 0; i < dim; ++i) {
                ret.start[i] += width;
                ret.end[i] -= width;
                ret.pace[i] -= 2 * width;
            }
            return ret;
        }

        void setEmpty() { *this = EmptyRange(); }

        auto first() const { return base_index_type {level, part, start}; }
        auto last() const {
            auto ret = base_index_type {level, part, end};
            for (int i = 0; i < d; ++i) ret[i]--;
            return ret;
        }

        bool empty() const { return count() <= 0; }

        bool is_divisible() const {
            for (int i = 0; i < dim; ++i) {
                if (end[i] - start[i] > 1) return true;
            }
            return false;
        }

        LevelRange(LevelRange& r, tbb::detail::split split) : LevelRange(r) {
            this->reValidPace();
            int max_iter = std::max_element(pace.begin(), pace.end()) - pace.begin();
            this->end[max_iter] = this->start[max_iter] + this->pace[max_iter] / 2;
            r.start[max_iter] = this->end[max_iter];
            this->pace[max_iter] /= 2;
            r.pace[max_iter] = r.end[max_iter] - r.start[max_iter];
        }

        LevelRange(LevelRange& r, tbb::detail::proportional_split proportion) : LevelRange(r) {
            this->reValidPace();
            int max_iter = std::max_element(pace.begin(), pace.end()) - pace.begin();
            int right_part = int(float(r.end[max_iter] - r.start[max_iter]) * float(proportion.right())
                                         / float(proportion.left() + proportion.right())
                                 + 0.5f);
            r.end[max_iter] -= right_part;
            this->start[max_iter] = r.end[max_iter];
            this->pace[max_iter] = this->end[max_iter] - this->start[max_iter];
            r.pace[max_iter] = r.end[max_iter] - r.start[max_iter];
        }

        static constexpr bool is_splittable_in_proportion = true;
        auto center() const {
            auto ret = base_index_type {level, part, start};
            for (int i = 0; i < d; ++i) ret[i] = (ret[i] + end[i]) / 2;
            return ret;
        }
    };
    template <std::size_t dim1, std::size_t dim2>
    constexpr auto commonRange(const LevelRange<dim1>& a, const LevelRange<dim2>& b) {
        std::array<int, dim1> start, end;
        if constexpr (dim1 != dim2) {
            static_assert(dim1 == 0 || dim2 == 0, OP_ERRMSG_DIM_MISMATCH);
            if constexpr (dim1 == 0) {
                return b;
            } else {
                return a;
            }
        } else {
            static_assert(dim1 == dim2, OP_ERRMSG_DIM_MISMATCH);
            OP_EXPECT(a.level == b.level);
            constexpr auto dim = dim1;
            start.fill(std::numeric_limits<int>::min());
            end.fill(std::numeric_limits<int>::max());
            for (auto i = 0; i < dim; ++i) {
                start[i] = std::max(a.start[i], b.start[i]);
                end[i] = std::min(a.end[i], b.end[i]);
            }
            auto ret = LevelRange<dim> {start, end};
            ret.level = a.level;
            ret.part = a.part;
            return ret;
        }
    }

    template <std::size_t dim>
    constexpr auto commonRanges(const std::vector<LevelRange<dim>>& a,
                                const std::vector<LevelRange<dim>>& b) {
        std::vector<LevelRange<dim>> ret;
        OP_ASSERT(a.size() == b.size());
        ret.reserve(a.size());
        for (auto i = 0; i < a.size(); ++i) { ret.push_back(commonRange(a[i], b[i])); }
        return ret;
    }

    template <std::size_t dim1, std::size_t dim2>
    constexpr auto mergeRange(const LevelRange<dim1>& a, const LevelRange<dim2>& b) {
        if constexpr (dim1 != dim2) {
            static_assert(dim1 == 0 || dim2 == 0, OP_ERRMSG_DIM_MISMATCH);
            if constexpr (dim1 == 0) return b;
            else
                return a;
        } else {
            static_assert(dim1 == dim2, OP_ERRMSG_DIM_MISMATCH);
            OP_ASSERT(a.level == b.level);
            constexpr auto dim = dim1;
            LevelRange<dim> ret;
            for (auto i = 0; i < dim; ++i) {
                ret.start[i] = std::max(a.start[i], b.start[i]);
                ret.end[i] = std::min(a.end[i], b.end[i]);
                OP_ASSERT(a.stride[i] == b.stride[i]);
            }
            ret.level = a.level;
            ret.part = a.part;
            return ret;
        }
    }

    template <std::size_t dim1, std::size_t dim2>
    constexpr auto minCoverRange(const LevelRange<dim1>& a, const LevelRange<dim2>& b) {
        if constexpr (dim1 != dim2) {
            static_assert(dim1 == 0 || dim2 == 0, OP_ERRMSG_DIM_MISMATCH);
            if constexpr (dim1 == 0) return b;
            else
                return a;
        } else {
            static_assert(dim1 == dim2, OP_ERRMSG_DIM_MISMATCH);
            OP_ASSERT(a.level == b.level && a.part == b.part);
            constexpr auto dim = dim1;
            LevelRange<dim> ret;
            for (auto i = 0; i < dim; ++i) {
                ret.start[i] = std::min(a.start[i], b.start[i]);
                ret.end[i] = std::max(a.end[i], b.end[i]);
                OP_ASSERT(a.stride[i] == b.stride[i]);
            }
            ret.level = a.level;
            ret.part = a.part;
            return ret;
        }
    }

    template <std::size_t dim>
    constexpr auto minCoverRange(const std::vector<LevelRange<dim>>& r) {
        if constexpr (dim == 0) return r[0];
        else {
            LevelRange<dim> ret = r[0];
            for (auto i = 1; i < r.size(); ++i) { ret = minCoverRange(ret, r[i]); }
            return ret;
        }
    }

    template <std::size_t dim1, std::size_t dim2, isRange... T>
    constexpr auto mergeRangeLists(const std::vector<LevelRange<dim1>>& r1,
                                   const std::vector<LevelRange<dim2>>& r2, T&&... rs) {
        if constexpr (dim1 == 0) {
            return mergeRangeLists(r2, std::forward<T>(rs)...);
        } else if constexpr (dim2 == 0) {
            return mergeRangeLists(r1, std::forward<T>(rs)...);
        } else {
            static_assert(dim1 == dim2, OP_ERRMSG_DIM_MISMATCH);
            std::vector<LevelRange<dim1>> ret;
            for (auto i = 0; i < r1.size(); ++i) { ret.push_back(mergeRange(r1[i], r2[i])); }
            return mergeRangeLists(ret, std::forward<T>(rs)...);
        }
    }

    template <std::size_t dim>
    constexpr auto mergeRangeLists(const std::vector<LevelRange<dim>>& r) {
        return r;
    }

    template <std::size_t dim, Meta::BracketIndexable T>
    constexpr auto inRange(const LevelRange<dim>& r, const T& t) {
        auto ret = true;
        for (auto i = 0; i < dim; ++i) { ret &= (r.start[i] <= t[i] && t[i] < r.end[i]); }
        return ret;
    }
    template <std::size_t dim>
    constexpr auto inRange(const LevelRange<dim>& a, const LevelRange<dim>& b) {
        // check if a in b
        OP_ASSERT(a.level == b.level);
        auto ret = true;
        for (auto i = 0; i < dim; ++i) { ret &= (a.start[i] >= b.start[i] && a.end[i] <= b.end[i]); }
        return ret;
    }
    template <std::size_t dim>
    constexpr auto intersectRange(const LevelRange<dim>& r1, const LevelRange<dim>& r2) {
        auto ret = false;
        for (auto i = 0; i < dim; ++i) { ret |= r1.start[i] >= r2.end[i] || r1.end[i] <= r2.start[i]; }
        return !ret;
    }

}// namespace OpFlow::DS
#endif//OPFLOW_LEVELRANGES_HPP
