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

#ifndef OPFLOW_LEVELMDRANGEMAPPER_HPP
#define OPFLOW_LEVELMDRANGEMAPPER_HPP

#include "DataStructures/Index/LevelMDIndex.hpp"
#include "DataStructures/Range/LevelRanges.hpp"
#ifndef OPFLOW_INSIDE_MODULE
#include <vector>
#endif

OPFLOW_MODULE_EXPORT namespace OpFlow::DS {
    template <std::size_t d>
    struct LevelMDRangeMapper {
        LevelMDRangeMapper() = default;
        explicit LevelMDRangeMapper(const std::vector<std::vector<LevelRange<d>>>& range) : _range(range) {
            calculateMultiplier();
        }

        const auto& getRange() const { return _range; }
        void setRange(const std::vector<std::vector<LevelRange<d>>>& range) {
            _range = range;
            calculateMultiplier();
        }

        int operator()(const LevelMDIndex<d>& idx) const {
            int ret = 0;
            for (auto i = 0; i < d; ++i) { ret += _fac[idx.l][idx.p][i] * idx[i]; }
            return ret + _offset[idx.l][idx.p];
        }

    private:
        void calculateMultiplier() {
            int temp_block_size = _range[0][0].count();
            _offset[0][0] = 0;
            for (auto l = 1; l < _range.size(); ++l) {
                _offset[l][0] = _offset[l - 1].back() + temp_block_size;
                for (auto p = 1; p < _range[l].size(); ++p) {
                    _offset[l][p] = _offset[l][p - 1] + _range[l][p - 1].count();
                }
            }
            for (auto l = 0; l < _range.size(); ++l) {
                for (auto p = 0; p < _range[l].size(); ++p) {
                    _fac[l][p][0] = 1;
                    for (auto i = 1; i < d; ++i) {
                        _fac[l][p][i]
                                = (_range[l][p].end[i - 1] - _range[l][p].start[i - 1]) * _fac[l][p][i - 1];
                    }
                }
            }
        }

        std::vector<std::vector<int>> _offset;
        std::vector<std::vector<std::array<int, d>>> _fac;
        std::vector<std::vector<LevelRange<d>>> _range;
    };
}// namespace OpFlow::DS

#endif//OPFLOW_LEVELMDRANGEMAPPER_HPP
