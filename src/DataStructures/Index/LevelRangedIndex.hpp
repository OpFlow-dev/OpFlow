//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2025 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_LEVELLevelRangedIndex_HPP
#define OPFLOW_LEVELLevelRangedIndex_HPP

#include "DataStructures/Index/LevelMDIndex.hpp"
#include "DataStructures/Range/LevelRanges.hpp"
#include "LevelMDIndex.hpp"

OPFLOW_MODULE_EXPORT namespace OpFlow::DS {
    template <std::size_t dim>
    struct LevelRangedIndex : public LevelMDIndex<dim> {
        DS::Range<dim> range;
        LevelRangedIndex() = default;

        constexpr explicit LevelRangedIndex(const DS::Range<dim>& range) : range(range) {
            this->set(this->range.start);
        }
        constexpr LevelRangedIndex(int l, int p, const DS::Range<dim>& range) : range(range) {
            this->l = l;
            this->p = p;
            this->set(this->range.start);
        }
        constexpr LevelRangedIndex(const DS::LevelRange<dim>& range) : range(range) {
            this->l = range.level;
            this->p = range.part;
            this->set(this->range.start);
        }

        using LevelMDIndex<dim>::operator[];

        constexpr auto next() const {
            LevelRangedIndex ret = *this;
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
            LevelRangedIndex ret = *this;
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
            LevelRangedIndex ret = *this;
            ret.set(this->range.start);
            return ret;
        }

        constexpr auto count() const {
            auto ret = 1;
            for (auto i = 0; i < dim; ++i) {
                ret *= (range.end[i] - range.start[i] - 1) / range.stride[i] + 1;
            }
            return ret;
        }

        constexpr auto last() const {
            LevelRangedIndex ret = *this;
            for (auto i = 0; i < dim; ++i) {
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
            for (auto i = 0; i < dim && k > 0; ++i) {
                off[i] = k % pace[i];
                k /= pace[i];
            }
            for (auto i = 0; i < dim; ++i) {
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
            for (auto i = 0; i < dim && k > 0; ++i) {
                off[i] = k % pace[i];
                k /= pace[i];
            }
            for (auto i = 0; i < dim; ++i) {
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
        /// Helper function to add 1 to dim k of a LevelRangedIndex
        /// \tparam k target dimension
        /// \param index input LevelRangedIndex
        /// \return modified LevelRangedIndex
        template <std::size_t k>
        constexpr void add1(LevelRangedIndex& index) const {
            static_assert(k <= dim, "LevelRangedIndex::add1's rank exceeds limit");
            if constexpr (k == dim) {
                // range exceeds the end, we make it stop at range.end
                index.set(index.range.end);
                return;
            }
            index[k] += index.range.stride[k];
            // we assert here dim k will at most add1 to dim k + 1
            OP_ASSERT(index[k] - index.range.end[k] < index.range.end[k] - index.range.start[k]);
            if (index[k] >= index.range.end[k]) {
                index[k] = index.range.start[k];
                // inserted to prevent infinity-template-loop
                if constexpr (k + 1 <= dim) add1<k + 1>(index);
            }
        }

        template <std::size_t k>
        constexpr void sub1(LevelRangedIndex& index) const {
            static_assert(k <= dim, "LevelRangedIndex::add1's rank exceeds limit");
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
    };
}// namespace OpFlow::DS

#endif//OPFLOW_LEVELLevelRangedIndex_HPP
