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

#ifndef OPFLOW_COLOREDMDRANGEMAPPER_HPP
#define OPFLOW_COLOREDMDRANGEMAPPER_HPP

#include "MDRangeMapper.hpp"

OPFLOW_MODULE_EXPORT namespace OpFlow::DS {
    template <std::size_t dim>
    struct ColoredMDRangeMapper {
        ColoredMDRangeMapper() = default;

        explicit ColoredMDRangeMapper(const Range<dim>& r, auto&&... rs) : ranges({r, rs...}) {
            init_mappers();
        }

        void init_mappers() {
            offset.resize(ranges.size() + 1);
            offset[0] = offset[1] = 0;
            for (int i = 2; i < offset.size(); ++i) { offset[i] = offset[i - 1] + ranges[i - 2].count(); }
            for (const auto& r : ranges) { mappers.template emplace_back(r); }
        }

        auto operator()(const ColoredIndex<MDIndex<dim>>& idx) const {
            return mappers[idx.color](idx) + offset[idx.color + 1];
        }

        auto getLocalRank(const ColoredIndex<MDIndex<dim>>& idx) const { return operator()(idx); }

        auto operator()(const MDIndex<dim>& idx, int i) const { return mappers[i](idx); }

        std::vector<Range<dim>> ranges;
        std::vector<MDRangeMapper<dim>> mappers;
        std::vector<int> offset;
    };
}// namespace OpFlow::DS

#endif//OPFLOW_COLOREDMDRANGEMAPPER_HPP
