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

#ifndef OPFLOW_TECPLOTBINARYSTREAM_HPP
#define OPFLOW_TECPLOTBINARYSTREAM_HPP

#include "Core/Field/MeshBased/Structured/CartesianField.hpp"
#include "TECIO.h"
#include "Utils/Writers/FieldStream.hpp"
#include <filesystem>
#include <fmt/format.h>
#include <string>

namespace OpFlow::Utils {
    struct TecplotBinaryStream;

    namespace internal {
        template <>
        struct StreamTrait<TecplotBinaryStream> {
            static constexpr auto mode_flag = StreamOut | StreamBinary;
        };
    }// namespace internal

    struct TecplotBinaryStream : FieldStream<TecplotBinaryStream> {
        TecplotBinaryStream() = default;
        explicit TecplotBinaryStream(const std::string& path) : path(path) {
            id = new_global_id();
            OP_ASSERT_MSG(id < 11, "TecplotBinaryStream: more than 10 streams are not supported.");
        }
        ~TecplotBinaryStream() { close(); }

        void close() {
            if (initialized) {
                tecfil142(&id);
                tecend142();
                initialized = false;
            }
        }

        auto& operator<<(const TimeStamp& t) {
            time = t;
            return *this;
        }

        std::string static commonSuffix() { return ".plt"; }

        void fixedMeshImpl() { _alwaysWriteMesh = false; }

        void dumpToSeparateFileImpl() { separate_file = true; }

        void useLogicalRange(bool o) { dumpLogicalRange = o; }

        static int new_global_id() {
            static int id = 1;
            return id++;
        }

        template <CartesianFieldExprType T>
        auto& operator<<(const T& f) {
            constexpr auto dim = OpFlow::internal::CartesianFieldExprTrait<T>::dim;
            constexpr bool isDouble
                    = std::is_same_v<typename OpFlow::internal::CartesianFieldExprTrait<T>::elem_type,
                                     double>;
            f.prepare();
            std::string name = f.getName();
            {
                auto ptr = std::remove(name.begin(), name.end(), ' ');
                name.erase(ptr, name.end());
            }
            std::string title = fmt::format("TITLE = \"Solution of {}\"", name);
            std::string var_list = (dim == 2) ? fmt::format("X,Y,{}", name) : fmt::format("X,Y,Z,{}", name);
            std::string filename = path;
            if (separate_file) {
                // add time stamp between filename and extension
                std::string ext = std::filesystem::path(path).extension();
                filename.erase(filename.end() - ext.size(), filename.end());
                filename += fmt::format("_{:.6f}", time.time);
                filename += ext;
            }
            int file_format = 0,                 // 0: Tecplot binary (.plt), 1: Tecplot subzone (.szplt)
                    file_type = 0,               // 0: full, 1: grid, 2: solution
                    debug = 0,                   // 0: no-debug, 1: debug
                    is_double = isDouble ? 1 : 0;// 0: f32, 1: f64
            int stat;
            if (!initialized) {
                std::filesystem::path dir = path;
                std::string parent_dir = dir.parent_path().string();
                stat = tecini142(title.c_str(), var_list.c_str(), filename.c_str(), parent_dir.c_str(),
                                 &file_format, &file_type, &debug, &is_double);
                OP_ASSERT_MSG(stat == 0, "TecplotBinaryStream: File init failed {}", filename);
                initialized = true;
            }
            tecfil142(&id);

            std::string zone_title = name;
            auto range = dumpLogicalRange ? f.logicalRange : f.localRange;
            int zone_type = 0, imax = range.end[0] - range.start[0],
                jmax = (dim >= 2) ? range.end[1] - range.start[1] : 1,
                kmax = (dim >= 3) ? range.end[2] - range.start[2] : 1, icellmax = 0, jcellmax = 0,
                kcellmax = 0, strandID = 1, parentZone = 0, isBlock = 1, dummy = 0;
            std::vector<int> passive_var(dim + 1, 0), share(dim + 1, 1);
            share.back() = 0;

            if (writeMesh) {
                teczne142(zone_title.c_str(), &zone_type, &imax, &jmax, &kmax, &icellmax, &jcellmax,
                          &kcellmax, &time.time, &strandID, &parentZone, &isBlock, &dummy, &dummy, &dummy,
                          &dummy, &dummy, passive_var.data(), nullptr, nullptr, &dummy);
                auto m = f.getMesh();
                const auto& loc = f.loc;
                for (auto k = 0; k < dim; ++k) {
                    std::vector<double> xs;
                    if (loc[k] == LocOnMesh::Corner) {
                        for (int iter = range.start[k]; iter < range.end[k]; ++iter) {
                            xs.push_back(m.x(k, iter));
                        }
                    } else {
                        for (int iter = range.start[k]; iter < range.end[k]; ++iter) {
                            xs.push_back(Math::mid(m.x(k, iter), m.x(k, iter + 1)));
                        }
                    }
                    rangeFor_s(range, [&](auto&& i) {
                        int N = 1;
                        int db = 1;
                        tecdat142(&N, (void*) (&xs[i[k] - range.start[k]]), &db);
                    });
                }
                if (!separate_file) writeMesh = _alwaysWriteMesh;
            } else {
                teczne142(zone_title.c_str(), &zone_type, &imax, &jmax, &kmax, &icellmax, &jcellmax,
                          &kcellmax, &time.time, &strandID, &parentZone, &isBlock, &dummy, &dummy, &dummy,
                          &dummy, &dummy, passive_var.data(), nullptr, share.data(), &dummy);
            }
            rangeFor_s(range, [&](auto&& i) {
                int N = 1;
                auto var = f[i];
                tecdat142(&N, (void*) &var, &is_double);
            });

            if (separate_file) close();
            return *this;
        }

        template <CartesianFieldExprType... Ts>
        auto& dumpMultiple(const Ts&... fs) {
            if constexpr (sizeof...(fs) == 1) {
                this->operator<<(OP_PERFECT_FOWD(fs)...);
                return *this;
            } else {
                constexpr auto dim = OpFlow::internal::CartesianFieldExprTrait<Meta::firstOf_t<Ts...>>::dim;
                constexpr bool isDouble = std::is_same_v<
                        typename OpFlow::internal::CartesianFieldExprTrait<Meta::firstOf_t<Ts...>>::elem_type,
                        double>;
                auto fs_tuple = std::make_tuple(&fs...);
                (fs.prepare(), ...);
                auto getName = [&](auto&& f) {
                    static int count = 0;
                    std::string name = f.getName();
                    if (name.empty()) name = fmt::format("unnamed{}", count++);
                    auto ptr = std::remove(name.begin(), name.end(), ' ');
                    name.erase(ptr, name.end());
                    std::replace(name.begin(), name.end(), ',', '_');
                    return name;
                };
                std::string title = fmt::format("TITLE = \"Solution of all fields\"");
                std::string var_list = (dim == 2) ? ("X,Y" + ... + fmt::format(",{}", getName(fs)))
                                                  : ("X,Y,Z" + ... + fmt::format(",{}", getName(fs)));
                std::string filename = path;
                if (separate_file) {
                    // add time stamp between filename and extension
                    std::string ext = std::filesystem::path(path).extension();
                    filename.erase(filename.end() - ext.size(), filename.end());
                    filename += fmt::format("_{:.6f}", time.time);
                    filename += ext;
                }
                int file_format = 0,                 // 0: Tecplot binary (.plt), 1: Tecplot subzone (.szplt)
                        file_type = 0,               // 0: full, 1: grid, 2: solution
                        debug = 0,                   // 0: no-debug, 1: debug
                        is_double = isDouble ? 1 : 0;// 0: f32, 1: f64
                int stat;
                if (!initialized) {
                    std::filesystem::path dir = path;
                    std::string parent_dir = dir.parent_path().string();
                    stat = tecini142(title.c_str(), var_list.c_str(), filename.c_str(), parent_dir.c_str(),
                                     &file_format, &file_type, &debug, &is_double);
                    OP_ASSERT_MSG(stat == 0, "TecplotBinaryStream: File init failed {}", filename);
                    initialized = true;
                }
                tecfil142(&id);

                std::string zone_title = "allinone";
                auto range = dumpLogicalRange ? maxCommonRange(std::vector {fs.logicalRange...})
                                              : maxCommonRange(std::vector {fs.localRange...});
                int zone_type = 0, imax = range.end[0] - range.start[0],
                    jmax = (dim >= 2) ? range.end[1] - range.start[1] : 1,
                    kmax = (dim >= 3) ? range.end[2] - range.start[2] : 1, icellmax = 0, jcellmax = 0,
                    kcellmax = 0, strandID = 1, parentZone = 0, isBlock = 1, dummy = 0;
                std::vector<int> passive_var(dim + sizeof...(fs), 0), share(dim + sizeof...(fs), 0);
                for (int i = 0; i < dim; ++i) share[i] = 1;

                if (writeMesh) {
                    teczne142(zone_title.c_str(), &zone_type, &imax, &jmax, &kmax, &icellmax, &jcellmax,
                              &kcellmax, &time.time, &strandID, &parentZone, &isBlock, &dummy, &dummy, &dummy,
                              &dummy, &dummy, passive_var.data(), nullptr, nullptr, &dummy);
                    auto m = std::get<0>(fs_tuple)->getMesh();
                    const auto& loc = (fs.loc, ...);
                    for (auto k = 0; k < dim; ++k) {
                        std::vector<double> xs;
                        if (loc[k] == LocOnMesh::Corner) {
                            for (int iter = range.start[k]; iter < range.end[k]; ++iter) {
                                xs.push_back(m.x(k, iter));
                            }
                        } else {
                            for (int iter = range.start[k]; iter < range.end[k]; ++iter) {
                                xs.push_back(Math::mid(m.x(k, iter), m.x(k, iter + 1)));
                            }
                        }
                        rangeFor_s(range, [&](auto&& i) {
                            int N = 1;
                            int db = 1;
                            tecdat142(&N, (void*) (&xs[i[k] - range.start[k]]), &db);
                        });
                    }
                    if (!separate_file) writeMesh = _alwaysWriteMesh;
                } else {
                    stat = teczne142(zone_title.c_str(), &zone_type, &imax, &jmax, &kmax, &icellmax,
                                     &jcellmax, &kcellmax, &time.time, &strandID, &parentZone, &isBlock,
                                     &dummy, &dummy, &dummy, &dummy, &dummy, passive_var.data(), nullptr,
                                     share.data(), &dummy);
                    OP_ASSERT_MSG(stat == 0, "TecplotBinaryStream: Zone init failed {}", zone_title);
                }

                Meta::static_for<sizeof...(fs)>([&]<int k>(Meta::int_<k>) {
                    rangeFor_s(range, [&](auto&& i) {
                        int N = 1;
                        auto var = std::get<k>(fs_tuple)->operator[](i);
                        tecdat142(&N, (void*) &var, &is_double);
                    });
                });

                if (separate_file) close();
                return *this;
            }
        }

    private:
        std::string path;
        TimeStamp time {};
        bool writeMesh = true, _alwaysWriteMesh = true, dumpLogicalRange = false, separate_file = false;
        bool initialized = false;
        int id;
    };
}// namespace OpFlow::Utils

#endif//OPFLOW_TECPLOTBINARYSTREAM_HPP
