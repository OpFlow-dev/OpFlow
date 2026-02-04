//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2026 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_RAWBINARYSTREAM_HPP
#define OPFLOW_RAWBINARYSTREAM_HPP

#include "Core/Environment.hpp"
#include "Core/Field/MeshBased/Structured/CartesianFieldExprTrait.hpp"
#include "Utils/Writers/FieldStream.hpp"
#include <format>
#ifndef OPFLOW_INSIDE_MODULE
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#endif

OPFLOW_MODULE_EXPORT namespace OpFlow::Utils {
    struct RawBinaryOStream;
    struct RawBinaryIStream;

    namespace internal {
        template <>
        struct StreamTrait<RawBinaryOStream> {
            static constexpr auto mode_flag = StreamOut | StreamBinary;
        };
        template <>
        struct StreamTrait<RawBinaryIStream> {
            static constexpr auto mode_flag = StreamIn | StreamBinary;
        };
    }// namespace internal

    struct RawBinaryOStream : FieldStream<RawBinaryOStream> {
        RawBinaryOStream() = default;
        explicit RawBinaryOStream(const std::filesystem::path& path) : path(path.parent_path()) {}

        // time info
        auto& operator<<(const TimeStamp& t) {
            time = t;
            return *this;
        }

        void setCounterTo(int c) { count = c; }

        // field writers
        template <CartesianFieldExprType T>
        RawBinaryOStream& operator<<(const T& f);

    private:
        std::filesystem::path path;
        TimeStamp time {};
        int count = 0;
    };

    struct RawBinaryIStream : FieldStream<RawBinaryIStream> {
        RawBinaryIStream() = default;
        explicit RawBinaryIStream(const std::filesystem::path& path) : path(path.parent_path()) {}

        void setCounterTo(int c) { count = c; }

        // field readers
        template <CartesianFieldType T>
        RawBinaryIStream& operator>>(T& f);

    private:
        std::filesystem::path path;
        int count = 0;
    };
}// namespace OpFlow::Utils

OPFLOW_MODULE_EXPORT namespace OpFlow::Utils {
    /// \brief Stream operator for CartesianField
    /// \note
    /// The structure of CartesianField raw file:
    /// <std::string> field name
    /// <int>         field dim
    /// <int>         total ranks
    /// zones info:
    /// <double>                        time stamp
    /// <dim * 2 * int>                 mesh range
    /// <n1 ... nd * double>            mesh coordinates
    /// <dim * 2 * int>                 field global range
    /// <dim * 2 * int>                 field local range
    /// <T * counts>                    field data
    /// \tparam T Input field type
    /// \param f Input field
    /// \return The ostream
    template <CartesianFieldExprType T>
    RawBinaryOStream& RawBinaryOStream::operator<<(const T& f) {
        static int nproc = 1, rank = 0;
        constexpr auto dim = OpFlow::internal::CartesianFieldExprTrait<T>::dim;

        // write meta & mesh info for the first time
        if (!getGlobalParallelPlan().singleNodeMode()) {
            nproc = getGlobalParallelPlan().distributed_workers_count;
            rank = getWorkerId();
        }
        std::string root;
        if (nproc > 1) root = std::format("{}/{}_{}_{}.cart", path.string(), f.getName(), count, rank);
        else
            root = std::format("{}/{}_{}.cart", path.string(), f.getName(), count);
        FILE* data;
        data = fopen(root.c_str(), "wb");
        std::string name = f.getName();
        int name_len = name.size();
        fwrite(&name_len, sizeof(int), 1, data);
        fwrite(name.c_str(), sizeof(char), name_len, data);
        fwrite(&dim, sizeof(int), 1, data);
        fwrite(&nproc, sizeof(int), 1, data);

        // zone info
        auto t = (double) time;
        fwrite(&t, sizeof(double), 1, data);
        auto& mesh = f.getMesh();
        auto mesh_range = mesh.getRange();
        for (auto i = 0; i < dim; ++i) {
            fwrite(&mesh_range.start[i], sizeof(mesh_range.start[i]), 1, data);
            fwrite(&mesh_range.end[i], sizeof(mesh_range.end[i]), 1, data);
        }
        for (auto i = 0; i < dim; ++i) {
            for (auto j = mesh_range.start[i]; j < mesh_range.end[i]; ++j) {
                auto x = mesh.x(i, j);
                fwrite(&x, sizeof(x), 1, data);
            }
        }
        for (auto i = 0; i < dim; ++i) {
            fwrite(&f.accessibleRange.start[i], sizeof(f.accessibleRange.start[i]), 1, data);
            fwrite(&f.accessibleRange.end[i], sizeof(f.accessibleRange.end[i]), 1, data);
        }
        for (auto i = 0; i < dim; ++i) {
            fwrite(&f.localRange.start[i], sizeof(f.localRange.start[i]), 1, data);
            fwrite(&f.localRange.end[i], sizeof(f.localRange.end[i]), 1, data);
        }
        rangeFor_s(f.localRange, [&](auto&& i) {
            auto x = f.evalSafeAt(i);
            fwrite(&x, sizeof(x), 1, data);
        });
        fclose(data);
        count++;
        return *this;
    }

    template <CartesianFieldType T>
    RawBinaryIStream& RawBinaryIStream::operator>>(T& f) {
        static int nproc = 1, rank = 0;
        constexpr auto dim = OpFlow::internal::CartesianFieldExprTrait<T>::dim;

        if (!getGlobalParallelPlan().singleNodeMode()) {
            nproc = getGlobalParallelPlan().distributed_workers_count;
            rank = getWorkerId();
        }
        std::string root;
        if (nproc > 1) root = std::format("{}/{}_{}_{}.cart", path.string(), f.getName(), count, rank);
        else
            root = std::format("{}/{}_{}.cart", path.string(), f.getName(), count);
        FILE* data;
        data = fopen(root.c_str(), "rb");

        // check meta infos are match
        std::string name;
        int name_len;
        fread(&name_len, sizeof(int), 1, data);
        char* name_c_str = new char[name_len + 1];
        fread(name_c_str, sizeof(char), name_len, data);
        name_c_str[name_len] = 0;
        name = std::string(name_c_str);
        delete[] name_c_str;
        OP_EXPECT_MSG(name == f.getName(), "Field's name {} in file is different from dst field {}", name,
                      f.getName());
        int f_dim;
        fread(&f_dim, sizeof(int), 1, data);
        OP_ASSERT_MSG(f_dim == dim, "Field read error: Dim mismatch {} != {}", f_dim, dim);
        int f_nproc;
        fread(&f_nproc, sizeof(int), 1, data);
        OP_ASSERT_MSG(f_nproc == nproc, "Field read error: data set nproc mismatch {} != {}", f_nproc, nproc);

        // zone info
        double t;
        fread(&t, sizeof(double), 1, data);
        auto m_range = f.mesh.getRange();
        for (auto i = 0; i < dim; ++i) {
            fread(&m_range.start[i], sizeof(m_range.start[i]), 1, data);
            fread(&m_range.end[i], sizeof(m_range.end[i]), 1, data);
        }
        OP_ASSERT_MSG(m_range == f.mesh.getRange(), "Field read error: Mesh range mismatch {} != {}",
                      m_range.toString(), f.mesh.getRange().toString());
        Real dummy;
        for (auto i = 0; i < dim; ++i) {
            for (auto j = m_range.start[i]; j < m_range.end[i]; ++j) {
                fread(&dummy, sizeof(dummy), 1, data);
                OP_ASSERT_MSG(dummy == f.mesh.x(i, j),
                              "Field read error: Mesh coordinate mismatch at x[{}][{}] {} != {}", i, j, dummy,
                              f.mesh.x(i, j));
            }
        }
        auto f_range = f.accessibleRange;
        for (auto i = 0; i < dim; ++i) {
            fread(&f_range.start[i], sizeof(f_range.start[i]), 1, data);
            fread(&f_range.end[i], sizeof(f_range.end[i]), 1, data);
        }
        OP_ASSERT_MSG(f_range == f.accessibleRange,
                      "Field read error: Field accessible range mismatch {} != {}", f_range.toString(),
                      f.accessibleRange.toString());
        for (auto i = 0; i < dim; ++i) {
            fread(&f_range.start[i], sizeof(f_range.start[i]), 1, data);
            fread(&f_range.end[i], sizeof(f_range.end[i]), 1, data);
        }
        OP_ASSERT_MSG(f_range == f.localRange, "Field read error: Field local range mismatch {} != {}",
                      f_range.toString(), f.localRange.toString());
        rangeFor_s(f.localRange, [&](auto&& i) { fread(&f[i], sizeof(f[i]), 1, data); });
        fclose(data);
        f.updatePadding();
        count++;
        return *this;
    }
}// namespace OpFlow::Utils

#endif//OPFLOW_RAWBINARYSTREAM_HPP
