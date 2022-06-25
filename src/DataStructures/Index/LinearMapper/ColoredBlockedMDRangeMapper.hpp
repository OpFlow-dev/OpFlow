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
        }

        void init_mappers() {
            // note: offset begins with index 1, which is due to that the color starts with 1
            offset.resize(ranges.size() + 1);
            offset[0] = offset[1] = 0;
            for (int i = 2; i < offset.size(); ++i) {
                int counts = 0;
                for (const auto& r : ranges[i - 2]) { counts += r.count(); }
                offset[i] = offset[i - 1] + counts;
            }
            for (const auto& r : ranges) { mappers.template emplace_back(r); }
        }

        auto operator()(const ColoredIndex<MDIndex<dim>>& idx) const {
            return mappers[idx.color](idx) + offset[idx.color + 1];
        }

        auto operator()(const MDIndex<dim>& idx, int i) const { return mappers[i](idx); }

        std::vector<std::vector<Range<dim>>> ranges;
        std::vector<BlockedMDRangeMapper<dim>> mappers;
        std::vector<int> offset;
    };
}// namespace OpFlow::DS

#endif//OPFLOW_COLOREDBLOCKEDMDRANGEMAPPER_HPP
