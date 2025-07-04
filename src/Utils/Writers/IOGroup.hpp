// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2025 by the OpFlow developers
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
#include "Utils/Writers/Streams.hpp"
#ifndef OPFLOW_INSIDE_MODULE
#include <tuple>
#endif

OPFLOW_MODULE_EXPORT namespace OpFlow::Utils {
    struct IOGroupInterface {
        virtual ~IOGroupInterface() = default;
        virtual void dump(const TimeStamp& t) = 0;
        virtual void read(const TimeStamp& t) {}
        virtual void setAllInOne(bool o) = 0;
        virtual void fixedMesh() {}
        virtual void dumpToSeparateFile() {}
        virtual void setNumberingType(NumberingType type) = 0;
    };

    template <typename Stream, typename... Exprs>
    struct IOGroup : public virtual IOGroupInterface {

        IOGroup(const IOGroup&) = default;
        IOGroup(IOGroup&&) noexcept = default;

        explicit IOGroup(const std::string& root, unsigned int mode, auto&&... es)
            : root(root), mode(mode), exprs(OP_PERFECT_FOWD(es)...) {}

        void init() {
            if (inited) return;
            if (allInOne && sizeof...(Exprs) > 1) {
                if constexpr (RStreamType<Stream> && WStreamType<Stream>)
                    streams.emplace_back(root + "AllInOne" + Stream::commonSuffix(), mode);
                else
                    streams.template emplace_back(root + "AllInOne" + Stream::commonSuffix());
            } else {
                Meta::static_for<sizeof...(Exprs)>([&]<int k>(Meta::int_<k>) {
                    auto& e = std::get<k>(exprs);
                    e.prepare();
                    std::string name = e.getName();
                    if (name.empty() || name.size() > 20) {
                        name = random_name(4);
                        e.name = "data";
                    }
                    if constexpr (RStreamType<Stream> && WStreamType<Stream>)
                        streams.emplace_back(root + name + Stream::commonSuffix(), mode);
                    else
                        streams.template emplace_back(root + name + Stream::commonSuffix());
                });
            }
            if (fixed_mesh) {
                if constexpr (requires(Stream s) { s.fixedMesh(); }) {
                    for (auto& s : streams) s.fixedMesh();
                } else {
                    OP_CRITICAL("IOGroup: stream does not support fixed mesh.");
                    OP_ABORT;
                }
            }
            if (separate_file) {
                if constexpr (requires(Stream s) { s.dumpToSeparateFile(); }) {
                    for (auto& s : streams) s.dumpToSeparateFile();
                } else {
                    OP_CRITICAL("IOGroup: stream does not support dump to separate file.");
                    OP_ABORT;
                }
            }
            if constexpr (requires(Stream s) { s.setNumberingType(NumberingType::ByTime); }) {
                for (auto& s : streams) s.setNumberingType(numberingType);
            } else {
                OP_CRITICAL("IOGroup: stream does not support set numbering type.");
                OP_ABORT;
            }
            inited = true;
        }

        void dump(const TimeStamp& t) override {
            if (!inited) init();
            if constexpr (WStreamType<Stream>) {
                if (allInOne) {
                    streams.back() << t;
                    [&, this]<int... Is>(std::integer_sequence<int, Is...>) {
                        streams.back().dumpMultiple(std::get<Is>(exprs)...);
                    }
                    (std::make_integer_sequence<int, sizeof...(Exprs)>());
                } else {
                    Meta::static_for<sizeof...(Exprs)>(
                            [&]<int k>(Meta::int_<k>) { streams[k] << t << std::get<k>(exprs); });
                }
            } else {
                OP_CRITICAL("IOGroup: stream not writable.");
                OP_ABORT;
            }
        }

        void read(const TimeStamp& t) override {
            if (!inited) init();
            if constexpr (RStreamType<Stream>)
                Meta::static_for<sizeof...(Exprs)>([&]<int k>(Meta::int_<k>) {
                    // there may be temporal expressions for output
                    if constexpr (Meta::RealType<decltype(std::get<k>(exprs))>::isConcrete())
                        streams[k].moveToTime(t) >> std::get<k>(exprs);
                });
            else {
                OP_CRITICAL("IOGroup: stream not readable.");
                OP_ABORT;
            }
        }

        void setAllInOne(bool o) override { allInOne = o; }

        void fixedMesh() override { fixed_mesh = true; }

        void dumpToSeparateFile() override { separate_file = true; }

        void setNumberingType(NumberingType type) override { numberingType = type; }

        std::tuple<typename std::conditional_t<
                RStreamType<Stream>,
                typename std::conditional_t<Exprs::isConcrete(), Meta::RealType<Exprs>&,
                                            Meta::RealType<Exprs>>,
                typename std::conditional_t<Exprs::isConcrete(), const Meta::RealType<Exprs>&,
                                            Meta::RealType<Exprs>>>...>
                exprs;
        std::vector<Stream> streams;
        bool allInOne = false, fixed_mesh = false, separate_file = false;
        std::string root;
        unsigned int mode;
        NumberingType numberingType = NumberingType::ByTime;
        bool inited = false;
    };

    template <typename Stream, ExprType... Exprs>
    auto makeIOGroup(const std::string& root, Exprs&&... es) {
        //static_assert((Meta::RealType<Exprs>::isConcrete() &&...));
        return IOGroup<Stream, Meta::RealType<Exprs>...>(root, StreamOut, OP_PERFECT_FOWD(es)...);
    }

    template <typename Stream, ExprType... Exprs>
    auto makeIOGroup(const std::string& root, unsigned int mode, Exprs&&... es) {
        return IOGroup<Stream, Meta::RealType<Exprs>...>(root, mode, OP_PERFECT_FOWD(es)...);
    }

    template <typename Stream, ExprType... Exprs>
    auto makeIOGroupPtr(const std::string& root, Exprs&&... es) {
        return new IOGroup<Stream, Meta::RealType<Exprs>...>(root, StreamOut, OP_PERFECT_FOWD(es)...);
    }

    template <typename Stream, ExprType... Exprs>
    auto makeIOGroupPtr(const std::string& root, unsigned int mode, Exprs&&... es) {
        return new IOGroup<Stream, Meta::RealType<Exprs>...>(root, mode, OP_PERFECT_FOWD(es)...);
    }
}// namespace OpFlow::Utils

#endif//OPFLOW_IOGROUP_HPP
