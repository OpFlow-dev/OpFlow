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
        explicit BlockedMDRangeMapper(const std::vector<Range<d>>& ranges) : _ranges(ranges) {
            gatherSplits();
            sortRanges();
            calculateOffsets();
            calculateMultiplier();
        }
        explicit BlockedMDRangeMapper(const Range<d>& range) {
#ifdef OPFLOW_WITH_MPI
            if (getWorkerCount() > 1) {
                _ranges.resize(getWorkerCount());
                _ranges[getWorkerId()] = range;
                std::vector<std::array<int, d>> _starts(_ranges.size()), _ends(_ranges.size()),
                        _strides(_ranges.size());
                _starts[getWorkerId()] = range.start;
                _ends[getWorkerId()] = range.end;
                _strides[getWorkerId()] = range.stride;
                MPI_Allgather(MPI_IN_PLACE, d, MPI_INT, _starts.data(), d, MPI_INT, MPI_COMM_WORLD);
                MPI_Allgather(MPI_IN_PLACE, d, MPI_INT, _ends.data(), d, MPI_INT, MPI_COMM_WORLD);
                MPI_Allgather(MPI_IN_PLACE, d, MPI_INT, _strides.data(), d, MPI_INT, MPI_COMM_WORLD);
                for (int i = 0; i < _ranges.size(); ++i) {
                    _ranges[i] = Range<d>(_starts[i], _ends[i], _strides[i]);
                }
            } else {
                _ranges.resize(1);
                _ranges[0] = range;
            }
#else
            _ranges.resize(1);
            _ranges[0] = range;
#endif
            gatherSplits();
            calculateOffsets();
            calculateMultiplier();
        }

        std::array<int, d> getBlockIndex(const MDIndex<d>& idx) const {
            if (!checkIndexInBlock(idx, getBlockRank(last_block))) {
                for (auto i = 0; i < d; ++i) {
                    for (auto j = 0; j < _split[i].size() - 1; ++j) {
                        if (_split[i][j] <= idx[i] && idx[i] < _split[i][j + 1]) {
                            last_block[i] = j;
                            break;
                        }
                    }
                }
            }
            return last_block;
        }

        bool checkIndexInBlock(const MDIndex<d>& idx, const int block_rank) const {
            return inRange(_ranges[block_rank], idx);
        }

        int getBlockRank(const std::array<int, d>& block_idx) const {
            // we assume the _ranges are listed by column-major sequence
            int block_rank = block_idx[d - 1];
            for (int i = d - 2; i >= 0; --i) {
                block_rank *= _split[i].size() - 1;
                block_rank += block_idx[i];
            }
            OP_ASSERT_MSG(block_rank < _ranges.size(), "Block rank {} out of range {}", block_rank,
                          _ranges.size());
            return block_rank;
        }

        int getBlockRank(const MDIndex<d>& idx) const { return getBlockRank(getBlockIndex(idx)); }

        int getBlockSize(int block_rank) const { return _ranges[block_rank].count(); }

        int getLocalRank(const MDIndex<d>& idx) const {
            if (!checkIndexInBlock(idx, last_block_rank)) {
                last_block = getBlockIndex(idx);
                last_block_rank = getBlockRank(last_block);
            }
            int block_rank = last_block_rank;
            OP_ASSERT_MSG(block_rank < _ranges.size(),
                          "BlockedMDRangeMapper error: block_rank {} large than total ranges count {}",
                          block_rank, _ranges.size());
            const auto& _r = _ranges[block_rank];
            OP_ASSERT_MSG(inRange(_r, idx), "BlockedMDRangeMapper Error: index {} not in blocked range {}",
                          idx, _r.toString());
            int ret = 0;
            for (auto i = 0; i < d; ++i) ret += _fac[block_rank][i] * (idx[i] - _r.start[i]);
            return ret;
        }

        int operator()(const MDIndex<d>& idx) const {
            if (!checkIndexInBlock(idx, last_block_rank)) {
                last_block = getBlockIndex(idx);
                last_block_rank = getBlockRank(last_block);
            }
            int block_rank = last_block_rank;
            const auto& _r = _ranges[block_rank];
            OP_ASSERT_MSG(inRange(_r, idx),
                          "BlockedMDRangeMapper Error: index {} not in blocked range {}, block_idx = {{{}, "
                          "{}}}, last_block_rank = {}",
                          idx, _r.toString(), last_block[0], last_block[1], last_block_rank);
            int ret = _offset[block_rank];
            for (auto i = 0; i < d; ++i) ret += _fac[block_rank][i] * (idx[i] - _r.start[i]);
            return ret;
        }

        [[nodiscard]] int count() const { return _offset.back(); }
        [[nodiscard]] int block_count() const { return _ranges.size(); }

        int operator()(const ColoredIndex<MDIndex<d>>& idx) const {
            // todo: color is ignored here. check this out
            return this->operator()(MDIndex<d> {idx});
        }

        // todo: workaround same as MDIndexMapper
        int operator()(const MDIndex<d>& idx, int) const { return operator()(idx); }

    private:
        void gatherSplits() {
            // IMPORTANT ASSUMPTION:
            // We assume here the input _ranges are generated by splitting a range by several orthogonal planes
            std::vector<std::vector<std::pair<int, int>>> segments(d);
            for (auto i = 0; i < d; ++i) {
                for (const auto& _r : _ranges) { segments[i].template emplace_back(_r.start[i], _r.end[i]); }
                std::sort(segments[i].begin(), segments[i].end());
                auto end = std::unique(segments[i].begin(), segments[i].end());
                segments[i].erase(end, segments[i].end());
                _split[i].push_back(segments[i].front().first);
                for (const auto& s : segments[i]) _split[i].push_back(s.second);
            }
        }

        void sortRanges() {
            // sort the ranges according to column-major rank
            std::sort(_ranges.begin(), _ranges.end(), [&](const Range<d>& a, const Range<d>& b) {
                // this happens when the left-most block is empty because all nodes covered by the boundary
                // e.g., x-face field on a 4x4 mesh with 3 MPI procs and Dirichlet boundary condition
                if (a.empty() && !b.empty()) {
                    return true;
                } else
                    return getBlockRank(getBlockIndex(a.first())) < getBlockRank(getBlockIndex(b.first()));
            });
        }

        void calculateOffsets() {
            _offset.resize(_ranges.size() + 1);
            _offset[0] = 0;
            for (auto i = 1; i < _offset.size(); ++i) {
                _offset[i] = _offset[i - 1] + _ranges[i - 1].count();
            }
        }

        void calculateMultiplier() {
            _fac.resize(_ranges.size());
            for (auto i = 0; i < _fac.size(); ++i) {
                _fac[i][0] = 1;
                for (auto j = 1; j < d; ++j) {
                    _fac[i][j] = (_ranges[i].end[j - 1] - _ranges[i].start[j - 1]) * _fac[i][j - 1];
                }
            }
        }

        std::vector<int> _offset;
        std::array<std::vector<int>, d> _split;
        std::vector<std::array<int, d>> _fac;
        std::vector<Range<d>> _ranges;

        // used to accelerate repeating query inside the same block
        mutable std::array<int, d> last_block {0};
        mutable int last_block_rank = 0;
    };
}// namespace OpFlow::DS

#endif//OPFLOW_BLOCKEDMDRANGEMAPPER_HPP
