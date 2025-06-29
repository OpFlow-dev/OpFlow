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

#ifndef OPFLOW_MDRANGEMAPPER_HPP
#define OPFLOW_MDRANGEMAPPER_HPP

#include "DataStructures/Index/MDIndex.hpp"
#include "DataStructures/Range/Ranges.hpp"

OPFLOW_MODULE_EXPORT namespace OpFlow::DS {
    template <std::size_t d>
    struct MDRangeMapper {
        MDRangeMapper() = default;
        explicit MDRangeMapper(const Range<d>& range) : _range(range) { calculateMultiplier(); }

        const auto& getRange() const { return _range; }
        void setRange(const Range<d>& range) {
            _range = range;
            calculateMultiplier();
        }

        // todo: temporary workaround for non-colored mappers operates like colored mapper's operator()(idx, i)
        int operator()(const MDIndex<d>& idx, int) const { return operator()(idx); }

        int operator()(const MDIndex<d>& idx) const {
            int ret = 0;
            for (std::size_t i = 0; i < d; ++i) ret += (idx[i] - _range.start[i]) * _fac[i];
            return ret;
        }

        int operator()(const ColoredIndex<MDIndex<d>>& idx) const {
            int ret = 0;
            for (std::size_t i = 0; i < d; ++i) ret += (idx[i] - _range.start[i]) * _fac[i];
            return ret;
        }

    private:
        void calculateMultiplier() {
            _fac[0] = 1;
            for (std::size_t i = 1; i < d; ++i) {
                _fac[i] = (_range.end[i - 1] - _range.start[i - 1]) * _fac[i - 1];
            }
        }

        Range<d> _range;
        std::array<int, d> _fac;
    };
}// namespace OpFlow::DS

#endif//OPFLOW_MDRANGEMAPPER_HPP
