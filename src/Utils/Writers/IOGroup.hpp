// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2022 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_IOGROUP_HPP
#define OPFLOW_IOGROUP_HPP

#include "Utils/RandomStringGenerator.hpp"
#include <tuple>

namespace OpFlow::Utils {
    template <typename Stream, typename... Exprs>
    struct IOGroup {

        IOGroup(const IOGroup&) = default;
        IOGroup(IOGroup&&) noexcept = default;

        explicit IOGroup(const std::string& root, unsigned int mode, Exprs&... es) : exprs(es...) {
            Meta::static_for<sizeof...(Exprs)>([&]<int k>(Meta::int_<k>) {
                std::string name = std::get<k>(exprs).getName();
                if (name.empty() || name.size() > 8) {
                    name = random_name(4);
                    std::get<k>(exprs).name = "data";
                }
                if constexpr (RStreamType<Stream> && WStreamType<Stream>)
                    streams.emplace_back(root + name + Stream::commonSuffix(), mode);
                else
                    streams.template emplace_back(root + name + Stream::commonSuffix());
            });
        }

        void dump(const TimeStamp& t) requires WStreamType<Stream> {
            Meta::static_for<sizeof...(Exprs)>(
                    [&]<int k>(Meta::int_<k>) { streams[k] << t << std::get<k>(exprs); });
        }

        void read(const TimeStamp& t) requires RStreamType<Stream> {
            Meta::static_for<sizeof...(Exprs)>(
                    [&]<int k>(Meta::int_<k>) { streams[k].moveToTime(t) >> std::get<k>(exprs); });
        }

        std::tuple<typename std::conditional_t<
                RStreamType<Stream>,
                typename std::conditional_t<Exprs::isConcrete(), Meta::RealType<Exprs>&,
                                            Meta::RealType<Exprs>>,
                typename OpFlow::internal::ExprProxy<Exprs>::type>...>
                exprs;
        std::vector<Stream> streams;
    };

    template <typename Stream, ExprType... Exprs>
    auto makeIOGroup(const std::string& root, Exprs&&... es) {
        return IOGroup<Stream, Meta::RealType<Exprs>...>(root, StreamOut, OP_PERFECT_FOWD(es)...);
    }

    template <typename Stream, ExprType... Exprs>
    auto makeIOGroup(const std::string& root, unsigned int mode, Exprs&&... es) {
        return IOGroup<Stream, Meta::RealType<Exprs>...>(root, mode, OP_PERFECT_FOWD(es)...);
    }
}// namespace OpFlow::Utils

#endif//OPFLOW_IOGROUP_HPP
