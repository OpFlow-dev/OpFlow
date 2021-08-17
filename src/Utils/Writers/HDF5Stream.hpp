//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2021  by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_HDF5STREAM_HPP
#define OPFLOW_HDF5STREAM_HPP

#ifdef OPFLOW_WITH_HDF5
#include "Core/Environment.hpp"
#include "Core/Field/MeshBased/Structured/CartesianFieldExprTrait.hpp"
#include "Utils/Writers/FieldStream.hpp"
#include "fmt/format.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <hdf5.h>
#include <string>
#include <utility>

namespace OpFlow::Utils {
    struct H5Stream;

    namespace internal {
        template <>
        struct StreamTrait<H5Stream> {
            static constexpr auto mode_flag = StreamOut | StreamBinary;
        };
    }// namespace internal

    struct H5Stream : FieldStream<H5Stream> {
        H5Stream() = default;
        explicit H5Stream(std::filesystem::path path, unsigned int mode = StreamOut)
            : path(std::move(path)), mode(mode) {
            OP_ASSERT_MSG(!this->path.filename().empty(), "H5Stream error: File name is empty");
#if defined(OPFLOW_WITH_MPI) && defined(OPFLOW_DISTRIBUTE_MODEL_MPI)
            MPI_Info info = MPI_INFO_NULL;
            auto fapl_id = H5Pcreate(H5P_FILE_ACCESS);
            H5Pset_fapl_mpio(fapl_id, mpi_comm, info);
            auto fcpl_id = H5P_DEFAULT;
#else
            auto fcpl_id = H5P_DEFAULT;
            auto fapl_id = H5P_DEFAULT;
#endif
            if (mode & StreamOut) file = H5Fcreate(this->path.c_str(), H5F_ACC_TRUNC, fcpl_id, fapl_id);
            else
                file = H5Fopen(this->path.c_str(), H5F_ACC_RDONLY, fapl_id);
            H5Pclose(fapl_id);
            file_inited = true;
        }
        ~H5Stream() = default;
        // time info
        auto& operator<<(const TimeStamp& t) {
            time = t;
            // create a new group
            if (!first_run) {
                H5Gclose(current_group);
            } else {
                first_run = false;
            }
            std::string group_name = fmt::format("/T={}", time.time);
            current_group = H5Gcreate(file, group_name.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            group_inited = true;
            return *this;
        }
        auto& moveToTime(const TimeStamp& t) {
            OP_ASSERT_MSG(mode & StreamIn, "H5Stream error: moveToTime can only be used under read mode");
            time = t;
            return *this;
        }

#if defined(OPFLOW_WITH_MPI) && defined(OPFLOW_DISTRIBUTE_MODEL_MPI)
        void setMPIComm(MPI_Comm comm) { mpi_comm = comm; }
#endif

        // field writers
        template <CartesianFieldExprType T>
        H5Stream& operator<<(const T& f);

        // field readers
        template <CartesianFieldExprType T>
        H5Stream& operator>>(T& f);

    private:
        std::filesystem::path path;
        hid_t file, current_group;
        TimeStamp time {0};
        bool first_run = true, file_inited = false, group_inited = false;
        unsigned int mode;
#ifdef OPFLOW_WITH_MPI
        MPI_Comm mpi_comm = MPI_COMM_WORLD;
#endif
    };

    template <CartesianFieldExprType T>
    H5Stream& H5Stream::operator<<(const T& f) {
        constexpr auto dim = OpFlow::internal::ExprTrait<T>::dim;
        using elem_type = typename OpFlow::internal::ExprTrait<T>::elem_type;

        if (!group_inited) *this << TimeStamp(0);
        // calculate the dim info of the data set
        auto extends = f.localRange.getExtends();
        auto global_extends = f.accessibleRange.getExtends();
        hsize_t h_extends[dim], h_global_extends[dim];
        for (auto i = 0; i < dim; ++i) {
            // column-major to row-major transpose
            h_extends[i] = extends[dim - i - 1];
            h_global_extends[i] = global_extends[dim - i - 1];
        }
        DS::PlainTensor<elem_type, dim> buffer(extends);
        DS::MDIndex<dim> _offset;
        for (auto i = 0; i < dim; ++i) _offset[i] = f.localRange.start[i];
        rangeFor(f.localRange, [&](auto&& i) { buffer[i - _offset] = f.evalAt(i); });
        auto dataspace = H5Screate_simple(OpFlow::internal::ExprTrait<T>::dim, h_global_extends, NULL);
        static_assert(Meta::Numerical<typename OpFlow::internal::ExprTrait<T>::elem_type>);
        hid_t datatype;

        if constexpr (std::is_same_v<elem_type, int>) datatype = H5Tcopy(H5T_NATIVE_INT);
        else if constexpr (std::is_same_v<elem_type, float>)
            datatype = H5Tcopy(H5T_NATIVE_FLOAT);
        else if constexpr (std::is_same_v<elem_type, double>)
            datatype = H5Tcopy(H5T_NATIVE_DOUBLE);
        else
            OP_CRITICAL("H5Stream fatal error: Field's element type not supported.");
        H5Tset_order(datatype, H5T_ORDER_LE);

        // create a new data set
        std::string dataset_name = fmt::format("/T={}/{}", time.time, f.getName());
        auto dataset = H5Dcreate(file, dataset_name.c_str(), datatype, dataspace, H5P_DEFAULT, H5P_DEFAULT,
                                 H5P_DEFAULT);
#if defined(OPFLOW_WITH_MPI) && defined(OPFLOW_DISTRIBUTE_MODEL_MPI)
        // write data by hyperslab
        auto mem_space = H5Screate_simple(OpFlow::internal::ExprTrait<T>::dim, h_extends, NULL);
        auto file_space = H5Dget_space(dataset);
        hsize_t offset[dim];
        // opflow uses column major layout, hdf5 uses row major layout
        for (auto i = 0; i < dim; ++i) offset[i] = f.localRange.start[dim - i - 1];
        H5Sselect_hyperslab(file_space, H5S_SELECT_SET, offset, NULL, h_extends, NULL);
        auto plist_id = H5Pcreate(H5P_DATASET_XFER);
        H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_INDEPENDENT);
        H5Dwrite(dataset, datatype, mem_space, file_space, plist_id, buffer.raw());
        H5Sclose(mem_space);
        H5Sclose(file_space);
        H5Pclose(plist_id);
#else
        // write data
        H5Dwrite(dataset, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer.raw());
#endif
        // close everything
        H5Dclose(dataset);
        H5Sclose(dataspace);

        return *this;
    }

    template <CartesianFieldExprType T>
    H5Stream& H5Stream::operator>>(T& f) {
        constexpr auto dim = OpFlow::internal::ExprTrait<T>::dim;
        using elem_type = typename OpFlow::internal::ExprTrait<T>::elem_type;

        // calculate the dim info of the data set
        auto extends = f.localRange.getExtends();
        auto global_extends = f.accessibleRange.getExtends();
        hsize_t h_extends[dim], h_global_extends[dim];
        for (auto i = 0; i < dim; ++i) {
            // column-major to row-major transpose
            h_extends[i] = extends[dim - i - 1];
            h_global_extends[i] = global_extends[dim - i - 1];
        }
        DS::PlainTensor<elem_type, dim> buffer(extends);
        DS::MDIndex<dim> _offset;
        for (auto i = 0; i < dim; ++i) _offset[i] = f.localRange.start[i];
        auto dataspace = H5Screate_simple(OpFlow::internal::ExprTrait<T>::dim, h_global_extends, NULL);
        static_assert(Meta::Numerical<typename OpFlow::internal::ExprTrait<T>::elem_type>);
        hid_t datatype;

        if constexpr (std::is_same_v<elem_type, int>) datatype = H5Tcopy(H5T_NATIVE_INT);
        else if constexpr (std::is_same_v<elem_type, float>)
            datatype = H5Tcopy(H5T_NATIVE_FLOAT);
        else if constexpr (std::is_same_v<elem_type, double>)
            datatype = H5Tcopy(H5T_NATIVE_DOUBLE);
        else
            OP_CRITICAL("H5Stream fatal error: Field's element type not supported.");
        H5Tset_order(datatype, H5T_ORDER_LE);

        // open the dataset
        std::string dataset_name = fmt::format("/T={}/{}", time.time, f.getName());
        auto dataset = H5Dopen(file, dataset_name.c_str(), H5P_DEFAULT);
#if defined(OPFLOW_WITH_MPI) && defined(OPFLOW_DISTRIBUTE_MODEL_MPI)
        // read data by hyperslab
        auto mem_space = H5Screate_simple(OpFlow::internal::ExprTrait<T>::dim, h_extends, NULL);
        auto file_space = H5Dget_space(dataset);
        hsize_t offset[dim];
        // opflow uses column major layout, hdf5 uses row major layout
        for (auto i = 0; i < dim; ++i) offset[i] = f.localRange.start[dim - i - 1];
        H5Sselect_hyperslab(file_space, H5S_SELECT_SET, offset, NULL, h_extends, NULL);
        auto plist_id = H5Pcreate(H5P_DATASET_XFER);
        H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_INDEPENDENT);
        H5Dread(dataset, datatype, mem_space, file_space, plist_id, buffer.raw());
        H5Sclose(mem_space);
        H5Sclose(file_space);
        H5Pclose(plist_id);
#else
        // read data
        H5Dread(dataset, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer.raw());
#endif
        // copy data from buffer to field
        rangeFor(f.localRange, [&](auto&& i) { f[i] = buffer[i - _offset]; });
        // close everything
        H5Dclose(dataset);
        H5Sclose(dataspace);

        return *this;
    }
}// namespace OpFlow::Utils
#endif

#endif//OPFLOW_HDF5STREAM_HPP
