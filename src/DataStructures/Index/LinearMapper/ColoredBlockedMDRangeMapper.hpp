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

#ifndef OPFLOW_COLOREDBLOCKEDMDRANGEMAPPER_HPP
#define OPFLOW_COLOREDBLOCKEDMDRANGEMAPPER_HPP

#include "BlockedMDRangeMapper.hpp"
#include "DataStructures/Index/ColoredIndex.hpp"

OPFLOW_MODULE_EXPORT namespace OpFlow::DS {
    /// @brief A mapper that maps a colored MDIndex to a linear index.
    /// @tparam dim The dimension of the MDIndex.
    /// @note The color lies in the outer-most dimension, i.e., the index picks the BlockedMDRangeMapper
    ///       by its color, then the BlockedMDRangeMapper tells the linear index.
    template <std::size_t dim>
    struct ColoredBlockedMDRangeMapper {
        ColoredBlockedMDRangeMapper() = default;

        explicit ColoredBlockedMDRangeMapper(const std::vector<Range<dim>>& r, auto&&... rs)
            : ranges({r, rs...}) {
            check_ranges();
            init_mappers();
            init_offset();
        }

        explicit ColoredBlockedMDRangeMapper(const Range<dim>& r, auto&&... rs) : localRanges({r, rs...}) {
#ifdef OPFLOW_WITH_MPI
            init_mappers_mpi();
#else
            ranges.resize(1);
            ranges[0] = localRanges;
            init_mappers();
#endif
            init_offset();
        }

    private:
        void init_offset() {
            int target_count = mappers.size();
            int block_counts = mappers[0].block_count();
            offset.resize(target_count);
            // loop over all blocks, calculate interleaved offset
            // target0.block0, ..., targetN.block0, target0.block1, ..., targetN.blockM
            // push back the initial 0
            offset[0].push_back(0);
            // initialize the 0th block
            for (int i = 1; i < target_count; ++i) {
                offset[i].push_back(offset[i - 1].back() + mappers[i - 1].getBlockSize(0));
            }
            // initialize block 1 ... M
            if (target_count == 1) {
                for (int b = 1; b < block_counts; ++b) {
                    offset[0].push_back(offset[0].back() + mappers[0].getBlockSize(b - 1));
                }
            } else {
                for (int b = 1; b < block_counts; ++b) {
                    offset[0].push_back(offset.back().back() + mappers.back().getBlockSize(b - 1));
                    for (int i = 1; i < target_count; ++i) {
                        // for each target i's block b, its offset is the offset of i-1's block b + its size
                        offset[i].push_back(offset[i - 1].back() + mappers[i - 1].getBlockSize(b));
                    }
                }
            }
            // local_offset records the offset of the blocks in the same block_rank, i.e.,
            // target0.block, target1.block, ..., targetN.block
            // the first index is the block rank, the second index is the target rank (contradictory to offset)
            local_offset.resize(block_counts);
            for (int b = 0; b < block_counts; ++b) {
                local_offset[b].push_back(0);
                for (int i = 1; i < target_count; ++i) {
                    local_offset[b].push_back(mappers[i - 1].getBlockSize(b));
                }
            }
        }

        void check_ranges() const {
            if (ranges.empty()) return;
            auto subranges_count = ranges[0].size();
            // all range list in ranges must have the same count of sub-ranges
            for (const auto& r : ranges) {
                OP_ASSERT_MSG(r.size() == subranges_count,
                              "ColoredBlockedMDRangeMapper: range list has {} elements which is not equal to "
                              "the common size {}",
                              r.size(), subranges_count);
            }
        }

        void init_mappers() {
            for (const auto& r : ranges) { mappers.emplace_back(r); }
        }

        void init_mappers_mpi() {
#ifdef OPFLOW_WITH_MPI
            for (const auto& r : localRanges) { mappers.emplace_back(r); }
#else
            init_mappers();
#endif
        }

    public:
        auto operator()(const ColoredIndex<MDIndex<dim>>& idx) const {
            return mappers[idx.color].getLocalRank(MDIndex<dim>(idx))
                   + offset[idx.color][mappers[idx.color].getBlockRank(MDIndex<dim>(idx))];
        }

        auto getLocalRank(const ColoredIndex<MDIndex<dim>>& idx) const {
            return mappers[idx.color].getLocalRank(MDIndex<dim>(idx))
                   + local_offset[mappers[idx.color].getBlockRank(MDIndex<dim>(idx))][idx.color];
        }

        auto operator()(const MDIndex<dim>& idx, int i) const { return mappers[i](idx); }

    private:
        std::vector<std::vector<Range<dim>>> ranges;
        std::vector<Range<dim>> localRanges;
        std::vector<BlockedMDRangeMapper<dim>> mappers;
        std::vector<std::vector<int>> offset, local_offset;
    };
}// namespace OpFlow::DS

#endif//OPFLOW_COLOREDBLOCKEDMDRANGEMAPPER_HPP
