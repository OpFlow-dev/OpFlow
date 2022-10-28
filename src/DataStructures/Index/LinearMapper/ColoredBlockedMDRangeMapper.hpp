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

#ifndef OPFLOW_COLOREDBLOCKEDMDRANGEMAPPER_HPP
#define OPFLOW_COLOREDBLOCKEDMDRANGEMAPPER_HPP

#include "BlockedMDRangeMapper.hpp"
#include "DataStructures/Index/ColoredIndex.hpp"

namespace OpFlow::DS {
    /// @brief A mapper that maps a colored MDIndex to a linear index.
    /// @tparam dim The dimension of the MDIndex.
    /// @note The color lies in the outer-most dimension, i.e., the index picks the BlockedMDRangeMapper
    ///       by its color, then the BlockedMDRangeMapper tells the linear index.
    template <std::size_t dim>
    struct ColoredBlockedMDRangeMapper {
        ColoredBlockedMDRangeMapper() = default;

        explicit ColoredBlockedMDRangeMapper(const std::vector<Range<dim>>& r, auto&&... rs)
            : ranges({r, rs...}) {
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
            offset.resize(mappers.size());
            offset[0] = 0;
            for (int i = 1; i < offset.size(); ++i) { offset[i] = offset[i - 1] + mappers[i - 1].count(); }
        }

        void init_mappers() {
            for (const auto& r : ranges) { mappers.template emplace_back(r); }
        }

        void init_mappers_mpi() {
#ifdef OPFLOW_WITH_MPI
            for (const auto& r : localRanges) { mappers.template emplace_back(r); }
#else
            init_mappers();
#endif
        }

    public:
        auto operator()(const ColoredIndex<MDIndex<dim>>& idx) const {
            return mappers[idx.color](idx) + offset[idx.color];
        }

        auto operator()(const MDIndex<dim>& idx, int i) const { return mappers[i](idx); }

        std::vector<std::vector<Range<dim>>> ranges;
        std::vector<Range<dim>> localRanges;
        std::vector<BlockedMDRangeMapper<dim>> mappers;
        std::vector<int> offset;
    };
}// namespace OpFlow::DS

#endif//OPFLOW_COLOREDBLOCKEDMDRANGEMAPPER_HPP
