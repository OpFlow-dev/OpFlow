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
#include "Utils/Writers/FieldStream.hpp"
#include <TECIO.h>
#include <filesystem>
#include <fmt/format.h>
#include <string>
#include <utility>

namespace OpFlow::Utils {
    struct TecplotBinaryStream;

    namespace internal {
        template <>
        struct StreamTrait<TecplotBinaryStream> {
            static constexpr auto mode_flag = StreamOut | StreamBinary;
        };
    }// namespace internal

    struct TecplotFileID {
        static int getID() {
            auto& ins = getInstance();
            OP_ASSERT_MSG(ins.ids.size() < 10, "TecplotFileID: More than 10 streams are not supported");
            ins.ids.push_back(newGlobalID());
            return ins.ids.back();
        }

        static int getCurrentRank(int id) {
            auto& ins = getInstance();
            auto iter = std::find(ins.ids.begin(), ins.ids.end(), id);
            if (iter == ins.ids.end()) {
                OP_CRITICAL("TecplotFileID: ID {} not found in record.", id);
                OP_ABORT;
            } else {
                return iter - ins.ids.begin() + 1;
            }
        }

        static void revokeID(int id) {
            auto& ins = getInstance();
            auto iter = std::find(ins.ids.begin(), ins.ids.end(), id);
            if (iter == ins.ids.end()) {
                return;
            } else {
                ins.ids.erase(iter);
            }
        }

    private:
        TecplotFileID() = default;
        TecplotFileID(const TecplotFileID&) = delete;
        TecplotFileID(TecplotFileID&&) = delete;
        TecplotFileID& operator=(const TecplotFileID&) = delete;

        static TecplotFileID& getInstance() {
            static TecplotFileID instance;
            return instance;
        }

        static int newGlobalID() {
            static int count = 1;
            return count++;
        }

        std::vector<int> ids;
    };

    struct TecplotBinaryStream : FieldStream<TecplotBinaryStream> {
        TecplotBinaryStream() = default;
        explicit TecplotBinaryStream(std::string path) : path(std::move(path)) {
            id = TecplotFileID::getID();
        }
        ~TecplotBinaryStream() { close(); }

        void close() {
            if (initialized) {
                int rank = TecplotFileID::getCurrentRank(id);
                tecfil142(&rank);
                tecend142();
                TecplotFileID::revokeID(id);
                initialized = false;
            }
        }

        auto& operator<<(const TimeStamp& t) {
            time = t;
            return *this;
        }

        std::string static commonSuffix() { return ".plt"; }

        void fixedMeshImpl() { fixed_mesh = false; }

        void dumpToSeparateFileImpl() {
            separate_file = true;
            if (!fixed_mesh) fixed_mesh = true;
        }

        void useLogicalRange(bool o) { dumpLogicalRange = o; }

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
            int file_format = 1,                 // 0: Tecplot binary (.plt), 1: Tecplot subzone (.szplt)
                    file_type = 0,               // 0: full, 1: grid, 2: solution
                    debug = 0,                   // 0: no-debug, 1: debug
                    is_double = isDouble ? 1 : 0;// 0: f32, 1: f64
            int stat;
            if (!initialized) {
                std::filesystem::path dir = path;
                std::string parent_dir = dir.parent_path().string();
                stat = tecini142(title.c_str(), var_list.c_str(), filename.c_str(), parent_dir.c_str(),
                                 &file_format, &file_type, &debug, &is_double);
#ifdef OPFLOW_WITH_MPI
                if (!getGlobalParallelPlan().singleNodeMode()) {
                    OP_ASSERT_MSG(dim == 3, "TecIO library only support partitioned IO for 3D data. Use "
                                            "other format or run in single node instead.");
                    auto comm = MPI_COMM_WORLD;
                    int master_rank = 0;
                    tecmpiinit142(&comm, &master_rank);
                }
#endif
                OP_ASSERT_MSG(stat == 0, "TecplotBinaryStream: File init failed {}", filename);
                initialized = true;
            }
            int rank = TecplotFileID::getCurrentRank(id);
            tecfil142(&rank);

            std::string zone_title = name;
            auto range = dumpLogicalRange ? f.logicalRange : f.localRange;
            auto globalRange = f.accessibleRange;
            int zone_type = 0, glob_i_count = globalRange.end[0] - globalRange.start[0],
                glob_j_count = (dim >= 2) ? globalRange.end[1] - globalRange.start[1] : 1,
                glob_k_count = (dim >= 3) ? globalRange.end[2] - globalRange.start[2] : 1,
                imin = range.start[0] - globalRange.start[0] + 1,
                jmin = (dim >= 2) ? range.start[1] - globalRange.start[1] + 1 : 1,
                kmin = (dim >= 3) ? range.start[2] - globalRange.start[2] + 1 : 1,
                imax = std::min(range.end[0] - range.start[0] + imin, glob_i_count),
                jmax = (dim >= 2) ? std::min(range.end[1] - range.start[1] + jmin, glob_j_count) : 1,
                kmax = (dim >= 3) ? std::min(range.end[2] - range.start[2] + kmin, glob_k_count) : 1,
                icellmax = 0, jcellmax = 0, kcellmax = 0, strandID = 1, parentZone = 0, isBlock = 1,
                dummy = 0, total_num_face_nodes = 1, nfconns = 0, fnmode = 0, total_num_boundary_faces = 1,
                total_num_boundary_connections = 1;
            std::vector<int> passive_var(dim + 1, 0), share(dim + 1, 1);
            share.back() = 0;
            auto extended_range = range;
            for (auto i = 0; i < dim; ++i)
                extended_range.end[i] = std::min(extended_range.end[i] + 1, globalRange.end[i]);
            {
                auto r = f.getLocalReadableRange();
                bool covered = true;
                for (int i = 0; i < dim; ++i) covered &= extended_range.end[i] <= r.end[i];
                OP_ASSERT_MSG(covered, "Field's extent must >= 1 to use TecIO's partitioned stream");
            }

            if (writeMesh) {
                teczne142(zone_title.c_str(), &zone_type, &glob_i_count, &glob_j_count, &glob_k_count,
                          &icellmax, &jcellmax, &kcellmax, &time.time, &strandID, &parentZone, &isBlock,
                          &nfconns, &fnmode, &total_num_face_nodes, &total_num_boundary_faces,
                          &total_num_boundary_connections, nullptr, nullptr, nullptr, &dummy);
#ifdef OPFLOW_WITH_MPI
                if (!getGlobalParallelPlan().singleNodeMode()) {
                    std::vector<int> partition_ranks(getWorkerCount());
                    for (auto i = 0; i < partition_ranks.size(); ++i) partition_ranks[i] = i;
                    int n_partitions = getWorkerCount(), current_partition = getWorkerId() + 1;
                    stat = tecznemap142(&n_partitions, partition_ranks.data());
                    stat = TECIJKPTN142(&current_partition, &imin, &jmin, &kmin, &imax, &jmax, &kmax);
                }
#endif
                auto m = f.getMesh();
                const auto& loc = f.loc;
                for (auto k = 0; k < dim; ++k) {
                    std::vector<double> xs;
                    if (loc[k] == LocOnMesh::Corner) {
                        for (int iter = extended_range.start[k]; iter < extended_range.end[k]; ++iter) {
                            xs.push_back(m.x(k, iter));
                        }
                    } else {
                        for (int iter = extended_range.start[k]; iter < extended_range.end[k]; ++iter) {
                            xs.push_back(Math::mid(m.x(k, iter), m.x(k, iter + 1)));
                        }
                    }
                    rangeFor_s(extended_range, [&](auto&& i) {
                        int N = 1;
                        int db = 1;
                        tecdat142(&N, (void*) (&xs[i[k] - extended_range.start[k]]), &db);
                    });
                }
                if (!separate_file) writeMesh = fixed_mesh;
            } else {
                teczne142(zone_title.c_str(), &zone_type, &glob_i_count, &glob_j_count, &glob_k_count,
                          &icellmax, &jcellmax, &kcellmax, &time.time, &strandID, &parentZone, &isBlock,
                          &dummy, &dummy, &dummy, &dummy, &dummy, passive_var.data(), nullptr, share.data(),
                          &dummy);
#ifdef OPFLOW_WITH_MPI
                if (!getGlobalParallelPlan().singleNodeMode()) {
                    std::vector<int> partition_ranks(getWorkerCount());
                    for (auto i = 0; i < partition_ranks.size(); ++i) partition_ranks[i] = i;
                    int n_partitions = getWorkerCount(), current_partition = getWorkerId() + 1;
                    tecznemap142(&n_partitions, partition_ranks.data());
                    tecijkptn142(&current_partition, &imin, &jmin, &kmin, &imax, &jmax, &kmax);
                }
#endif
            }
            rangeFor_s(extended_range, [&](auto&& i) {
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
                int file_format = 1,                 // 0: Tecplot binary (.plt), 1: Tecplot subzone (.szplt)
                        file_type = 0,               // 0: full, 1: grid, 2: solution
                        debug = 0,                   // 0: no-debug, 1: debug
                        is_double = isDouble ? 1 : 0;// 0: f32, 1: f64
                int stat;
                if (!initialized) {
                    std::filesystem::path dir = path;
                    std::string parent_dir = dir.parent_path().string();
                    stat = tecini142(title.c_str(), var_list.c_str(), filename.c_str(), parent_dir.c_str(),
                                     &file_format, &file_type, &debug, &is_double);
#ifdef OPFLOW_WITH_MPI
                    if (!getGlobalParallelPlan().singleNodeMode()) {
                        OP_ASSERT_MSG(dim == 3, "TecIO library only support partitioned IO for 3D data. Use "
                                                "other format or run in single node instead.");
                        auto comm = MPI_COMM_WORLD;
                        int master_rank = 0;
                        TECMPIINIT142(&comm, &master_rank);
                    }
#endif
                    OP_ASSERT_MSG(stat == 0, "TecplotBinaryStream: File init failed {}", filename);
                    initialized = true;
                }
                int rank = TecplotFileID::getCurrentRank(id);
                tecfil142(&rank);

                std::string zone_title = "allinone";
                auto range = dumpLogicalRange ? maxCommonRange(std::vector {fs.logicalRange...})
                                              : maxCommonRange(std::vector {fs.localRange...});
                auto globalRange = maxCommonRange(std::vector {fs.accessibleRange...});
                int zone_type = 0, glob_i_count = globalRange.end[0] - globalRange.start[0],
                    glob_j_count = (dim >= 2) ? globalRange.end[1] - globalRange.start[1] : 1,
                    glob_k_count = (dim >= 3) ? globalRange.end[2] - globalRange.start[2] : 1,
                    imin = range.start[0] - globalRange.start[0] + 1,
                    jmin = (dim >= 2) ? range.start[1] - globalRange.start[1] + 1 : 1,
                    kmin = (dim >= 3) ? range.start[2] - globalRange.start[2] + 1 : 1,
                    imax = std::min(range.end[0] - range.start[0] + imin, glob_i_count),
                    jmax = (dim >= 2) ? std::min(range.end[1] - range.start[1] + jmin, glob_j_count) : 1,
                    kmax = (dim >= 3) ? std::min(range.end[2] - range.start[2] + kmin, glob_k_count) : 1,
                    icellmax = 0, jcellmax = 0, kcellmax = 0, strandID = 1, parentZone = 0, isBlock = 1,
                    dummy = 0, total_num_face_nodes = 1, nfconns = 0, fnmode = 0, total_num_boundary_faces = 1,
                    total_num_boundary_connections = 1;
                std::vector<int> passive_var(dim + sizeof...(fs), 0), share(dim + sizeof...(fs), 0);
                for (int i = 0; i < dim; ++i) share[i] = 1;
                auto extended_range = range;
                for (auto i = 0; i < dim; ++i)
                    extended_range.end[i] = std::min(extended_range.end[i] + 1, globalRange.end[i]);
                {
                    auto r = maxCommonRange(std::vector {fs.getLocalReadableRange()...});
                    bool covered = true;
                    for (int i = 0; i < dim; ++i) covered &= extended_range.end[i] <= r.end[i];
                    OP_ASSERT_MSG(covered, "Field's extent must >= 1 to use TecIO's partitioned stream");
                }

                if (writeMesh) {
                    teczne142(zone_title.c_str(), &zone_type, &glob_i_count, &glob_j_count, &glob_k_count,
                              &icellmax, &jcellmax, &kcellmax, &time.time, &strandID, &parentZone, &isBlock,
                              &nfconns, &fnmode, &total_num_face_nodes, &total_num_boundary_faces,
                              &total_num_boundary_connections, nullptr, nullptr, nullptr, &dummy);
#ifdef OPFLOW_WITH_MPI
                    if (!getGlobalParallelPlan().singleNodeMode()) {
                        std::vector<int> partition_ranks(getWorkerCount());
                        for (auto i = 0; i < partition_ranks.size(); ++i) partition_ranks[i] = i;
                        int n_partitions = getWorkerCount(), current_partition = getWorkerId() + 1;
                        stat = tecznemap142(&n_partitions, partition_ranks.data());
                        stat = TECIJKPTN142(&current_partition, &imin, &jmin, &kmin, &imax, &jmax, &kmax);
                    }
#endif
                    auto m = std::get<0>(fs_tuple)->getMesh();
                    const auto& loc = (fs.loc, ...);
                    for (auto k = 0; k < dim; ++k) {
                        std::vector<double> xs;
                        if (loc[k] == LocOnMesh::Corner) {
                            for (int iter = extended_range.start[k]; iter < extended_range.end[k]; ++iter) {
                                xs.push_back(m.x(k, iter));
                            }
                        } else {
                            for (int iter = extended_range.start[k]; iter < extended_range.end[k]; ++iter) {
                                xs.push_back(Math::mid(m.x(k, iter), m.x(k, iter + 1)));
                            }
                        }
                        rangeFor_s(extended_range, [&](auto&& i) {
                            int N = 1;
                            int db = 1;
                            tecdat142(&N, (void*) (&xs[i[k] - extended_range.start[k]]), &db);
                        });
                    }
                    if (!separate_file) writeMesh = fixed_mesh;
                } else {
                    teczne142(zone_title.c_str(), &zone_type, &glob_i_count, &glob_j_count, &glob_k_count,
                              &icellmax, &jcellmax, &kcellmax, &time.time, &strandID, &parentZone, &isBlock,
                              &dummy, &dummy, &dummy, &dummy, &dummy, passive_var.data(), nullptr, share.data(),
                              &dummy);
#ifdef OPFLOW_WITH_MPI
                    if (!getGlobalParallelPlan().singleNodeMode()) {
                        std::vector<int> partition_ranks(getWorkerCount());
                        for (auto i = 0; i < partition_ranks.size(); ++i) partition_ranks[i] = i;
                        int n_partitions = getWorkerCount(), current_partition = getWorkerId() + 1;
                        tecznemap142(&n_partitions, partition_ranks.data());
                        tecijkptn142(&current_partition, &imin, &jmin, &kmin, &imax, &jmax, &kmax);
                    }
#endif
                }

                Meta::static_for<sizeof...(fs)>([&]<int k>(Meta::int_<k>) {
                    rangeFor_s(extended_range, [&](auto&& i) {
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
        bool writeMesh = true, fixed_mesh = true, dumpLogicalRange = false, separate_file = false;
        bool initialized = false;
        int id = -1;
    };
}// namespace OpFlow::Utils

#endif//OPFLOW_TECPLOTBINARYSTREAM_HPP
