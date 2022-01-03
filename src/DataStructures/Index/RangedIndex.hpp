//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2022 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_RANGEDINDEX_HPP
#define OPFLOW_RANGEDINDEX_HPP

#include "Core/Macros.hpp"
#include "DataStructures/Range/Ranges.hpp"
#include "MDIndex.hpp"

namespace OpFlow::DS {

    template <std::size_t dim>
    struct RangedIndex : public MDIndex<dim> {
        RangedIndex() = default;

        constexpr explicit RangedIndex(const DS::Range<dim>& range) : range(range) {
            this->set(this->range.start);
        }

        using MDIndex<dim>::operator[];

        constexpr auto next() const {
            RangedIndex ret = *this;
            add1<0>(ret);
            return ret;
        }

        template <std::size_t d>
        constexpr auto next() const {
            auto ret = *this;
            ret[d] += 1;
            return ret;
        }

        constexpr auto prev() const {
            RangedIndex ret = *this;
            sub1<0>(ret);
            return ret;
        }

        template <std::size_t d>
        constexpr auto prev() const {
            auto ret = *this;
            ret[d] -= 1;
            return ret;
        }

        /// The first valid index
        /// \return this->range.start
        constexpr auto first() const {
            RangedIndex ret = *this;
            ret.set(this->range.start);
            return ret;
        }

        constexpr auto count() const {
            auto ret = 1;
            for (std::size_t i = 0; i < dim; ++i) {
                ret *= (range.end[i] - range.start[i] - 1) / range.stride[i] + 1;
            }
            return ret;
        }

        constexpr auto last() const {
            RangedIndex ret = *this;
            for (std::size_t i = 0; i < dim; ++i) {
                ret[i] = ((range.end[i] - range.start[i] - 1) / range.stride[i]) * range.stride[i]
                         + range.start[i];
            }
            return ret;
        }

        constexpr auto& operator++() {
            add1<0>(*this);
            return *this;
        }

        constexpr const auto operator++(int) {
            auto ret = *this;
            add1<0>(*this);
            return ret;
        }

        constexpr auto& operator+=(int k) {
            std::array<int, dim> off;
            off.fill(0);
            const auto& pace = range.getPace();
            for (std::size_t i = 0; i < dim && k > 0; ++i) {
                off[i] = k % pace[i];
                k /= pace[i];
            }
            for (std::size_t i = 0; i < dim; ++i) {
                if (this->idx[i] + off[i] >= range.end[i]) {
                    this->idx[i] = this->idx[i] + off[i] - pace[i];
                    if (i + 1 == dim) {
                        this->set(range.end);
                        break;
                    }
                    this->idx[i + 1] += 1;
                } else {
                    this->idx[i] += off[i];
                }
            }
            return *this;
        }

        constexpr auto& operator-=(int k) {
            std::array<int, dim> off;
            off.fill(0);
            const auto& pace = range.getPace();
            for (std::size_t i = 0; i < dim && k > 0; ++i) {
                off[i] = k % pace[i];
                k /= pace[i];
            }
            for (std::size_t i = 0; i < dim; ++i) {
                if (this->idx[i] - off[i] < range.start[i]) {
                    this->idx[i] = this->idx[i] - off[i] + pace[i];
                    if (i + 1 == dim) {
                        this->set(range.start);
                        break;
                    }
                    this->idx[i + 1] -= 1;
                } else {
                    this->idx[i] -= off[i];
                    break;
                }
            }
            return *this;
        }

        constexpr auto& operator--() {
            sub1<0>(*this);
            return *this;
        }

        constexpr const auto operator--(int) {
            auto ret = *this;
            sub1<0>(*this);
            return ret;
        }

    private:
        /// Helper function to add 1 to dim k of a RangedIndex
        /// \tparam k target dimension
        /// \param index input RangedIndex
        /// \return modified RangedIndex
        template <std::size_t k>
        constexpr void add1(RangedIndex& index) const {
            static_assert(k <= dim, "RangedIndex::add1's rank exceeds limit");
            if constexpr (k == dim) {
                // range exceeds the end, we make it stop at range.end
                index.set(index.range.end);
                return;
            }
            index[k] += index.range.stride[k];
            // we assert here dim k will at most add1 to dim k + 1
#ifndef NDEBUG
            if (index[k] - index.range.end[k] >= index.range.end[k] - index.range.start[k]) {
                OP_ERROR("Index {} add more than 1 to dim {}", index, k);
                OP_ERROR("Index range = {}", index.range.toString());
            }
#endif
            OP_ASSERT(index[k] - index.range.end[k] < index.range.end[k] - index.range.start[k]);
            if (index[k] >= index.range.end[k]) {
                index[k] = index.range.start[k];
                // inserted to prevent infinity-template-loop
                if constexpr (k + 1 <= dim) add1<k + 1>(index);
            }
        }

        template <std::size_t k>
        constexpr void sub1(RangedIndex& index) const {
            static_assert(k <= dim, "RangedIndex::add1's rank exceeds limit");
            if constexpr (k == dim) {
                // range exceeds the start, we make it stop at range.start
                index.set(index.range.start);
                return;
            }
            index[k] -= index.range.stride[k];
            // we assert here dim k will at most borrow 1 from dim k + 1
            OP_ASSERT(index.range.start[k] - index[k] < index.range.end[k] - index.range.start[k]);
            if (index[k] < index.range.start[k]) {
                index[k] = index.range.start[k]
                           + (index.range.end[k] - index.range.start[k] - 1) / index.range.stride[k]
                                     * index.range.stride[k];
                if constexpr (k + 1 <= dim) sub1<k + 1>(index);
            }
        }

    public:
        DS::Range<dim> range;
    };
}// namespace OpFlow::DS

#endif//OPFLOW_RANGEDINDEX_HPP
