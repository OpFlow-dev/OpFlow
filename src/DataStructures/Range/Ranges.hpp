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

#ifndef OPFLOW_RANGES_HPP
#define OPFLOW_RANGES_HPP

#include "Core/Macros.hpp"
#include "DataStructures/Index/MDIndex.hpp"
#include "Utils/Serializer/STDContainers.hpp"
#include "Utils/xxHash.hpp"
#ifndef OPFLOW_INSIDE_MODULE
#include <array>
#include <concepts>
#include <cstddef>
#include <functional>
#include <oneapi/tbb/detail/_range_common.h>
#include <vector>
#endif

OPFLOW_MODULE_EXPORT

namespace OpFlow::DS
{
    template <std::size_t dim>
    struct RangedIndex;

    template <std::size_t d>
    struct Range
    {
        constexpr static int dim = d;
        using index_type = RangedIndex<d>;
        using base_index_type = MDIndex<d>;
        std::array<int, d> start, end, stride; // stride here means the step in each dim.
        // Not the same as StrideIndex.
    private:
        mutable std::array<int, d> pace;

    public:
        constexpr static auto EmptyRange()
        {
            Range ret;
            ret.start.fill(0);
            ret.end.fill(0);
            ret.stride.fill(1);
            ret.pace.fill(0);
            return ret;
        }

        constexpr Range() { stride.fill(1); }

        constexpr explicit Range(const std::array<int, d>& end) noexcept : end(end), pace(end)
        {
            start.fill(0);
            stride.fill(1);
        }

        constexpr Range(const std::array<int, d>& start, const std::array<int, d>& end) noexcept
            : start(start), end(end)
        {
            stride.fill(1);
            reValidPace();
        }

        constexpr Range(const std::array<int, d>& start, const std::array<int, d>& end,
                        const std::array<int, d>& stride) noexcept
            : start(start), end(end), stride(stride)
        {
            reValidPace();
        }

        constexpr void reValidPace() const
        {
            for (std::size_t i = 0; i < dim; ++i) pace[i] = (end[i] - start[i]) / stride[i];
        }

        constexpr auto operator==(const Range& other) const
        {
            return start == other.start && end == other.end && stride == other.stride;
        }

        constexpr auto check() const
        {
            bool ret = true;
            ret &= (d == start.size()) && (d == end.size()) && (d == stride.size());
            for (std::size_t i = 0; i < d; ++i)
            {
                ret &= (end[i] >= start[i]);
                ret &= (stride[i] >= 1);
            }
            return ret;
        }

        constexpr auto count() const
        {
            // make sure the pace is valid
            reValidPace();
            for (std::size_t i = 0; i < d; ++i)
            {
                if (pace[i] <= 0) return 0;
            }
            auto ret = 1;
            for (std::size_t i = 0; i < d; ++i) { ret *= pace[i]; }
            return ret;
        }

        constexpr auto covers(const Range& other) const
        {
            auto ret = true;
            for (std::size_t i = 0; i < d; ++i)
            {
                ret &= start[i] <= other.start[i] && end[i] >= other.end[i];
            }
            return ret;
        }

        constexpr const auto& getPace() const { return pace; }

        /// \brief Get the sliced range (1 layer thick) along dim \p k at position \p pos
        /// \param k normal dimension of the slice
        /// \param pos position of the slice
        constexpr auto slice(std::size_t k, int pos) const
        {
            assert(k < d);
            Range ret = *this;
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
        constexpr auto slice(std::size_t k, int pos_start, int pos_end) const
        {
            assert(k < d);
            Range ret = *this;
            ret.start[k] = pos_start;
            ret.end[k] = pos_end;
            ret.pace[k] = pos_end - pos_start;
            return ret;
        }

        auto toString() const
        {
            std::string ret;
            ret = std::format("{{{} - {} by {}}}", Utils::Serializer::serialize(start),
                              Utils::Serializer::serialize(end), Utils::Serializer::serialize(stride));
            return ret;
        }

        constexpr auto getExtends() const
        {
            reValidPace();
            std::array < int, d > ret;
            for (std::size_t i = 0; i < d; ++i) { ret[i] = pace[i]; }
            return ret;
        }

        constexpr auto getOffset() const
        {
            std::array < int, d > ret;
            for (std::size_t i = 0; i < d; ++i) ret[i] = start[i];
            return ret;
        }

        auto getBCRanges(int width) const
        {
            std::vector<Range> ret;
            for (std::size_t i = 0; i < d; ++i)
            {
                ret.push_back(*this);
                ret.back().end[i] = ret.back().start[i] + width;
                ret.back().pace[i] = width;
                ret.push_back(*this);
                ret.back().start[i] = ret.back().end[i] - width;
                ret.back().pace[i] = width;
            }
            return ret;
        }

        auto getInnerRange(int width) const
        {
            auto ret = *this;
            for (std::size_t i = 0; i < dim; ++i)
            {
                ret.start[i] += width;
                ret.end[i] -= width;
                ret.pace[i] -= 2 * width;
            }
            return ret;
        }

        void setEmpty() { *this = EmptyRange(); }

        auto first() const { return base_index_type{start}; }

        auto last() const
        {
            auto ret = base_index_type{end};
            for (int i = 0; i < d; ++i) ret[i]--;
            return ret;
        }

        bool empty() const { return count() <= 0; }

        bool is_divisible() const
        {
            for (int i = 0; i < dim; ++i)
            {
                if (end[i] - start[i] > 1) return true;
            }
            return false;
        }

        Range(Range& r, [[maybe_unused]] tbb::detail::split split) : Range(r)
        {
            this->reValidPace();
            int max_iter = std::max_element(pace.begin(), pace.end()) - pace.begin();
            this->end[max_iter] = this->start[max_iter] + this->pace[max_iter] / 2;
            r.start[max_iter] = this->end[max_iter];
            this->pace[max_iter] /= 2;
            r.pace[max_iter] = r.end[max_iter] - r.start[max_iter];
        }

        Range(Range& r, tbb::detail::proportional_split proportion) : Range(r)
        {
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

        auto center() const
        {
            auto ret = base_index_type{start};
            for (std::size_t i = 0; i < d; ++i) ret[i] = (ret[i] + end[i]) / 2;
            return ret;
        }
    };

    template <typename T>
    concept isRange = requires
        {
            T::dim;
        }
        && std::is_same_v<std::remove_cvref_t<T>, Range<T::dim>>;

    template <std::size_t dim1, std::size_t dim2>
    constexpr auto commonRange(const Range<dim1>& a, const Range<dim2>& b)
    {
        std::array < int, dim1 > start, end;
        if constexpr (dim1 != dim2)
        {
            static_assert(dim1 == 0 || dim2 == 0,
            OP_ERRMSG_DIM_MISMATCH
            )
            ;
            if constexpr (dim1 == 0)
            {
                return b;
            }
            else
            {
                return a;
            }
        }
        else
        {
            static_assert(dim1 == dim2,
            OP_ERRMSG_DIM_MISMATCH
            )
            ;
            constexpr auto dim = dim1;
            start.fill(std::numeric_limits<int>::min());
            end.fill(std::numeric_limits<int>::max());
            for (std::size_t i = 0; i < dim; ++i)
            {
                start[i] = std::max(a.start[i], b.start[i]);
                end[i] = std::min(a.end[i], b.end[i]);
            }
            return Range<dim>{start, end};
        }
    }

    template <std::size_t dim>
    constexpr auto commonRanges(const std::vector<Range<dim>>& a, const std::vector<Range<dim>>& b)
    {
        std::vector<Range<dim>> ret;
        OP_ASSERT(a.size() == b.size());
        ret.reserve(a.size());
        for (auto i = 0; i < a.size(); ++i) { ret.push_back(commonRange(a[i], b[i])); }
        return ret;
    }

    template <std::size_t dim1, std::size_t dim2>
    constexpr auto minCoverRange(const Range<dim1>& a, const Range<dim2>& b)
    {
        if constexpr (dim1 != dim2)
        {
            static_assert(dim1 == 0 || dim2 == 0,
            OP_ERRMSG_DIM_MISMATCH
            )
            ;
            if constexpr (dim1 == 0) return b;
            else
                return a;
        }
        else
        {
            static_assert(dim1 == dim2,
            OP_ERRMSG_DIM_MISMATCH
            )
            ;
            constexpr auto dim = dim1;
            Range<dim> ret;
            for (std::size_t i = 0; i < dim; ++i)
            {
                ret.start[i] = std::min(a.start[i], b.start[i]);
                ret.end[i] = std::max(a.end[i], b.end[i]);
                OP_ASSERT(a.stride[i] == b.stride[i]);
            }
            return Range<dim>{ret.start, ret.end};
        }
    }

    template <std::size_t dim>
    constexpr auto minCoverRange(const std::vector<Range<dim>>& r)
    {
        if constexpr (dim == 0) return r[0];
        else
        {
            Range<dim> ret = r[0];
            for (auto i = 1; i < r.size(); ++i) { ret = minCoverRange(ret, r[i]); }
            return ret;
        }
    }

    template <std::size_t dim1, std::size_t dim2>
    constexpr auto maxCommonRange(const Range<dim1>& a, const Range<dim2>& b)
    {
        if constexpr (dim1 != dim2)
        {
            static_assert(dim1 == 0 || dim2 == 0,
            OP_ERRMSG_DIM_MISMATCH
            )
            ;
            if constexpr (dim1 == 0) return b;
            else
                return a;
        }
        else
        {
            static_assert(dim1 == dim2,
            OP_ERRMSG_DIM_MISMATCH
            )
            ;
            constexpr auto dim = dim1;
            Range<dim> ret;
            for (std::size_t i = 0; i < dim; ++i)
            {
                ret.start[i] = std::max(a.start[i], b.start[i]);
                ret.end[i] = std::min(a.end[i], b.end[i]);
                OP_ASSERT(a.stride[i] == b.stride[i]);
                if (ret.end[i] < ret.start[i]) return Range<dim>{};
            }
            return Range<dim>{ret.start, ret.end};
        }
    }

    template <std::size_t dim>
    constexpr auto maxCommonRange(const std::vector<Range<dim>>& r)
    {
        if constexpr (dim == 0) return r[0];
        else
        {
            Range<dim> ret = r[0];
            for (auto i = 1; i < r.size(); ++i) { ret = maxCommonRange(ret, r[i]); }
            return ret;
        }
    }

    template <std::size_t dim, Meta::BracketIndexable T>
    constexpr auto inRange(const Range<dim>& r, const T& t)
    {
        auto ret = true;
        for (std::size_t i = 0; i < dim; ++i) { ret &= (r.start[i] <= t[i] && t[i] < r.end[i]); }
        return ret;
    }
} // namespace OpFlow::DS

template <std::size_t dim>
struct std::hash<OpFlow::DS::Range<dim>>
{
public:
    std::size_t operator()(const OpFlow::DS::Range<dim>& r) const noexcept
    {
        std::array < int, 3 * dim > arr;
        for (std::size_t i = 0; i < dim; ++i)
        {
            arr[i] = r.start[i];
            arr[i + dim] = r.end[i];
            arr[i + 2 * dim] = r.stride[i];
        }
        return XXHash64::hash(arr.data(), sizeof(arr), 0);
    }
};
#endif//OPFLOW_RANGES_HPP