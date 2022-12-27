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

#ifndef OPFLOW_GENERALBlockedMDRangeMapper_HPP
#define OPFLOW_GENERALBlockedMDRangeMapper_HPP

#include "DataStructures/Range/Ranges.hpp"
#include <algorithm>
#include <array>
#include <vector>

namespace OpFlow::DS {
    /**
     * \brief Axis aligned split multi-dimensional range mapper
     * @tparam d Dimension
     */
    template <std::size_t d>
    struct GeneralBlockedMDRangeMapper {
        GeneralBlockedMDRangeMapper() = default;

        // local ctor
        explicit GeneralBlockedMDRangeMapper(const std::vector<Range<d>>& ranges) : _ranges(ranges) {
            calculateOffsets();
            calculateMultiplier();
        }

        // MPI collective ctor
        explicit GeneralBlockedMDRangeMapper(const Range<d>& range) {
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
            calculateOffsets();
            calculateMultiplier();
        }

        bool checkIndexInBlock(const MDIndex<d>& idx, const int block_rank) const {
            return inRange(_ranges[block_rank], idx);
        }

        int getBlockRank(const MDIndex<d>& idx) const {
            if (!checkIndexInBlock(idx, last_block_rank)) {
                // loop over all ranges
                for (int i = 0; i < _ranges.size(); ++i) {
                    if (inRange(_ranges[i], idx)) {
                        last_block_rank = i;
                        return i;
                    }
                }
                OP_CRITICAL("Index {} not in any of the ranges", idx);
                OP_ABORT;
            } else
                return last_block_rank;
        }

        int getBlockSize(int block_rank) const { return _ranges[block_rank].count(); }

        int getLocalRank(const MDIndex<d>& idx) const {
            if (!checkIndexInBlock(idx, last_block_rank)) { last_block_rank = getBlockRank(idx); }
            int block_rank = last_block_rank;
            OP_ASSERT_MSG(block_rank < _ranges.size(),
                          "GeneralBlockedMDRangeMapper error: block_rank {} large than total ranges count {}",
                          block_rank, _ranges.size());
            const auto& _r = _ranges[block_rank];
            OP_ASSERT_MSG(inRange(_r, idx),
                          "GeneralBlockedMDRangeMapper Error: index {} not in blocked range {}", idx,
                          _r.toString());
            int ret = 0;
            for (auto i = 0; i < d; ++i) ret += _fac[block_rank][i] * (idx[i] - _r.start[i]);
            return ret;
        }

        int operator()(const MDIndex<d>& idx) const {
            if (!checkIndexInBlock(idx, last_block_rank)) { last_block_rank = getBlockRank(idx); }
            int block_rank = last_block_rank;
            const auto& _r = _ranges[block_rank];
            OP_ASSERT_MSG(inRange(_r, idx),
                          "GeneralBlockedMDRangeMapper Error: index {} not in blocked range {}", idx,
                          _r.toString());
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
        std::vector<std::array<int, d>> _fac;
        std::vector<Range<d>> _ranges;

        // used to accelerate repeating query inside the same block
        mutable int last_block_rank = 0;
    };
}// namespace OpFlow::DS

#endif//OPFLOW_GeneralBlockedMDRangeMapper_HPP
