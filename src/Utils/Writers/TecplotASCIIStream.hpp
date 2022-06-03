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

#ifndef OPFLOW_TECPLOTASCIISTREAM_HPP
#define OPFLOW_TECPLOTASCIISTREAM_HPP

#include "Core/Field/MeshBased/Structured/CartesianField.hpp"
#include "Utils/Writers/FieldStream.hpp"
#include "fmt/format.h"
#include <fstream>
#include <string>

namespace OpFlow::Utils {
    struct TecplotASCIIStream;

    namespace internal {
        template <>
        struct StreamTrait<TecplotASCIIStream> {
            static constexpr auto mode_flag = StreamOut | StreamASCII;
        };
    }// namespace internal
    struct TecplotASCIIStream : FieldStream<TecplotASCIIStream> {
        TecplotASCIIStream() = default;
        explicit TecplotASCIIStream(const std::string& path) : path(path) {
            of.open(path, std::ofstream::out | std::ofstream::ate);
        }

        // settings
        auto& operator<<(const TimeStamp& t) {
            time = t;
            return *this;
        }

        std::string static commonSuffix() { return ".tec"; }

        void alwaysWriteMesh(bool o) { _alwaysWriteMesh = o; }

        template <CartesianFieldExprType T>
        auto& operator<<(const T& f) {
            constexpr auto dim = OpFlow::internal::CartesianFieldExprTrait<T>::dim;
            f.prepare();
            if (of.tellp() == 0) {
                of << fmt::format("TITLE = \"Solution of {} \"\n", f.name);
                if constexpr (dim == 1) of << fmt::format("VARIABLES = {}, \"{}\"\n", R"("X")", f.name);
                else if constexpr (dim == 2)
                    of << fmt::format("VARIABLES = {}, \"{}\"\n", R"("X", "Y")", f.name);
                else if constexpr (dim == 3)
                    of << fmt::format("VARIABLES = {}, \"{}\"\n", R"("X", "Y", "Z")", f.name);
            }
            of << "ZONE\n";
            of << "ZONETYPE = ORDERED DATAPACKING = BLOCK\n";
            if constexpr (dim == 1)
                of << fmt::format("I = {}\n", f.localRange.end[0] - f.localRange.start[0]);
            else if constexpr (dim == 2)
                of << fmt::format("I = {} J = {}\n", f.localRange.end[0] - f.localRange.start[0],
                                  f.localRange.end[1] - f.localRange.start[1]);
            else if constexpr (dim == 3)
                of << fmt::format("I = {} J = {} K = {}\n", f.localRange.end[0] - f.localRange.start[0],
                                  f.localRange.end[1] - f.localRange.start[1],
                                  f.localRange.end[2] - f.localRange.start[2]);
            of << std::scientific << std::setprecision(10);
            of << fmt::format("SOLUTIONTIME = {}\n", time.time);
            if (!writeMesh) {
                if constexpr (dim == 1) of << "VARSHARELIST=([1]=1)\n";
                else
                    of << fmt::format("VARSHARELIST=([1-{}]=1)\n", dim);
            } else {
                auto m = f.getMesh();
                const auto& loc = f.loc;
                for (auto k = 0; k < dim; ++k) {
                    if (loc[k] == LocOnMesh::Corner)
                        rangeFor_s(f.localRange, [&](auto&& i) { of << m.x(k, i) << "\n"; });
                    else
                        rangeFor_s(f.localRange, [&](auto&& i) {
                            of << Math::mid(m.x(k, i[k]), m.x(k, i[k] + 1)) << "\n";
                        });
                }
            }
            rangeFor_s(f.localRange, [&](auto&& i) { of << f.evalAt(i) << "\n"; });

            of.flush();
            writeMesh = _alwaysWriteMesh;
            return *this;
        }

        template <CartesianFieldExprType... Ts>
        auto& dumpMultiple(const Ts&... fs) {
            if constexpr (sizeof...(fs) == 1) {
                this->operator<<(OP_PERFECT_FOWD(fs)...);
                return *this;
            } else {
                auto fs_tuple = std::make_tuple(&fs...);
                constexpr auto dim = OpFlow::internal::CartesianFieldExprTrait<Meta::firstOf_t<Ts...>>::dim;
                (fs.prepare(), ...);
                auto getName = [&](auto&& f) {
                    static int count = 0;
                    std::string name = f.getName();
                    if (name.empty()) name = fmt::format("unnamed{}", count++);
                    auto ptr = std::remove(name.begin(), name.end(), ' ');
                    name.erase(ptr, name.end());
                    return name;
                };
                if (of.tellp() == 0) {
                    of << fmt::format("TITLE = \"Solution of AllInOne\"\n");
                    if constexpr (dim == 1)
                        of << (fmt::format("VARIABLES = {}", R"("X")") + ...
                               + fmt::format(",\"{}\"", getName(fs)))
                           << "\n";
                    else if constexpr (dim == 2)
                        of << (fmt::format("VARIABLES = {}", R"("X", "Y")") + ...
                               + fmt::format(",\"{}\"", getName(fs)))
                           << "\n";
                    else if constexpr (dim == 3)
                        of << (fmt::format("VARIABLES = {}", R"("X", "Y", "Z")") + ...
                               + fmt::format(",\"{}\"", getName(fs)))
                           << "\n";
                }
                of << "ZONE\n";
                of << "ZONETYPE = ORDERED DATAPACKING = BLOCK\n";
                auto range = dumpLogicalRange ? maxCommonRange(std::vector {fs.logicalRange...})
                                              : maxCommonRange(std::vector {fs.localRange...});

                if constexpr (dim == 1) of << fmt::format("I = {}\n", range.end[0] - range.start[0]);
                else if constexpr (dim == 2)
                    of << fmt::format("I = {} J = {}\n", range.end[0] - range.start[0],
                                      range.end[1] - range.start[1]);
                else if constexpr (dim == 3)
                    of << fmt::format("I = {} J = {} K = {}\n", range.end[0] - range.start[0],
                                      range.end[1] - range.start[1], range.end[2] - range.start[2]);
                of << std::scientific << std::setprecision(10);
                of << fmt::format("SOLUTIONTIME = {}\n", time.time);
                if (!writeMesh) {
                    if constexpr (dim == 1) of << "VARSHARELIST=([1]=1)\n";
                    else
                        of << fmt::format("VARSHARELIST=([1-{}]=1)\n", dim);
                } else {
                    auto m = std::get<0>(fs_tuple)->getMesh();
                    const auto& loc = std::get<0>(fs_tuple)->loc;
                    for (auto k = 0; k < dim; ++k) {
                        if (loc[k] == LocOnMesh::Corner)
                            rangeFor_s(range, [&](auto&& i) { of << m.x(k, i) << "\n"; });
                        else
                            rangeFor_s(range, [&](auto&& i) {
                                of << Math::mid(m.x(k, i[k]), m.x(k, i[k] + 1)) << "\n";
                            });
                    }
                }
                Meta::static_for<sizeof...(fs)>([&]<int k>(Meta::int_<k>) {
                    rangeFor_s(range, [&](auto&& i) { of << std::get<k>(fs_tuple)->evalAt(i) << "\n"; });
                });

                of.flush();
                writeMesh = _alwaysWriteMesh;
                return *this;
            }
        }

    private:
        std::string path;
        std::ofstream of;
        TimeStamp time {};
        bool writeMesh = true, _alwaysWriteMesh = false, dumpLogicalRange = false;
    };
}// namespace OpFlow::Utils
#endif//OPFLOW_TECPLOTASCIISTREAM_HPP
