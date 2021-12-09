// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2021 by the OpFlow developers
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

        void alwaysWriteMesh(bool o) { _alwaysWriteMesh = o; }

        template <CartesianFieldExprType T>
        auto& operator<<(const T& f) {
            constexpr auto dim = OpFlow::internal::CartesianFieldExprTrait<T>::dim;
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

    private:
        std::string path;
        std::ofstream of;
        TimeStamp time {};
        bool writeMesh = true, _alwaysWriteMesh = false;
    };
}// namespace OpFlow::Utils
#endif//OPFLOW_TECPLOTASCIISTREAM_HPP
