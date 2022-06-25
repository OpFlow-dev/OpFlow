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

#ifndef OPFLOW_BLOCKEDMDRANGEMAPPER_HPP
#define OPFLOW_BLOCKEDMDRANGEMAPPER_HPP

#include "DataStructures/Range/Ranges.hpp"
#include <algorithm>
#include <array>
#include <vector>

namespace OpFlow::DS {
    template <std::size_t d>
    struct BlockedMDRangeMapper {
        BlockedMDRangeMapper() = default;
        explicit BlockedMDRangeMapper(const std::vector<Range<d>>& ranges) : _ranges(ranges) { calculateMultiplier(); }

        int operator()(const MDIndex<d>& idx) const {
            // we assume the _ranges are listed by column-major sequence
            int block_idx[d];
            for (auto i = 0; i < d; ++i) {
                for (auto j = 0; j < _split[i].size() - 1; ++j) {
                    if (_split[i][j] <= idx[i] && idx[i] < _split[i][j + 1]) {
                        block_idx[i] = j;
                        break;
                    }
                }
            }
            int block_rank = block_idx[d - 1];
            for (int i = d - 2; i >= 0; --i) {
                block_rank *= _split[i].size() - 1;
                block_rank += block_idx[i];
            }
            OP_ASSERT_MSG(block_rank < _ranges.size(), "Block rank {} out of range {}", block_rank, _ranges.size());
            const auto& _r = _ranges[block_rank];
            OP_ASSERT_MSG(inRange(_r, idx), "BlockedMDRangeMapper Error: index {} not in blocked range {}",
                          idx, _r.toString());
            int ret = _offset[block_rank];
            for (auto i = 0; i < d; ++i) ret += _fac[block_rank][i] * (idx[i] - _r.start[i]);
            return ret;
        }

        int operator()(const ColoredIndex<MDIndex<d>>& idx) const {
            // todo: color is ignored here. check this out
            return this->operator()(MDIndex<d> {idx});
        }

        // todo: workaround same as MDIndexMapper
        int operator()(const MDIndex<d>& idx, int) const { return operator()(idx); }

    private:
        void calculateMultiplier() {
            _offset.resize(_ranges.size());
            _offset[0] = 0;
            for (auto i = 1; i < _offset.size(); ++i) {
                _offset[i] = _offset[i - 1] + _ranges[i - 1].count();
            }

            _fac.resize(_ranges.size());
            for (auto i = 0; i < _fac.size(); ++i) {
                _fac[i][0] = 1;
                for (auto j = 1; j < d; ++j) {
                    _fac[i][j] = (_ranges[i].end[j - 1] - _ranges[i].start[j - 1]) * _fac[i][j - 1];
                }
            }

            // IMPORTANT ASSUMPTION:
            // We assume here the input _ranges are generated by splitting a range by several orthogonal planes
            for (auto i = 0; i < d; ++i) { _split[i].push_back(_ranges[0].start[i]); }
            for (const auto& _r : _ranges) {
                for (auto i = 0; i < d; ++i) _split[i].push_back(_r.end[i]);
            }
            for (auto i = 0; i < d; ++i) {
                std::sort(_split[i].begin(), _split[i].end());
                auto end = std::unique(_split[i].begin(), _split[i].end());
                _split[i].erase(end, _split[i].end());
            }
        }

        std::vector<int> _offset;
        std::array<std::vector<int>, d> _split;
        std::vector<std::array<int, d>> _fac;
        std::vector<Range<d>> _ranges;
    };
}// namespace OpFlow::DS

#endif//OPFLOW_BLOCKEDMDRANGEMAPPER_HPP
