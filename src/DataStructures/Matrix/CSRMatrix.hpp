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

#ifndef OPFLOW_CSRMATRIX_HPP
#define OPFLOW_CSRMATRIX_HPP

#include "Core/BasicDataTypes.hpp"
#include <fmt/format.h>
#include <oneapi/tbb/parallel_for.h>

namespace OpFlow::DS {
    struct CSRMatrix {
        CSRMatrix() = default;
        CSRMatrix(int n_row, int nnz_per_row) { resize(n_row, nnz_per_row); }

        void reserve(int n_row, int nnz_per_row) {
            row.reserve(n_row + 1);
            col.reserve(n_row * nnz_per_row);
            val.reserve(n_row * nnz_per_row);
            rhs.reserve(n_row);
        }

        void resize(int n_row, int nnz_per_row) {
            row.resize(n_row + 1);
            col.resize(n_row * nnz_per_row);
            val.resize(n_row * nnz_per_row);
            rhs.resize(n_row);
        }

        void trim(int nnz) {
            col.resize(nnz);
            val.resize(nnz);
        }

        auto& operator()(int r, int c) {
            auto iter = std::find(col.begin() + row[r], col.begin() + row[r + 1], c);
            if (iter == col.begin() + row[r + 1]) {
                OP_CRITICAL("CSRMatrix error: element don't exist at {{{}, {}}}", r, c);
                OP_ABORT;
            }
            return val[iter - col.begin()];
        }

        auto& operator()(int r, int c) const {
            auto iter = std::find(col.begin() + row[r], col.begin() + row[r + 1], c);
            if (iter == col.begin() + row[r + 1]) {
                OP_CRITICAL("CSRMatrix error: element don't exist at {{{}, {}}}", r, c);
                OP_ABORT;
            }
            return val[iter - col.begin()];
        }

        [[nodiscard]] int nnz() const { return val.size(); }

        void append(const CSRMatrix& mat) {
            int base_row = row.size(), base_rhs = rhs.size(), offset = row.back();
            row.resize(row.size() + mat.row.size() - 1);
            rhs.resize(rhs.size() + mat.rhs.size());
            oneapi::tbb::parallel_for(0, (int) mat.row.size() - 1,
                                      [&](int k) { row[base_row + k] = mat.row[k + 1] + offset; });
            oneapi::tbb::parallel_for(0, (int) mat.rhs.size(),
                                      [&](int k) { rhs[base_rhs + k] = mat.rhs[k]; });
            int base_col = col.size();
            col.resize(col.size() + mat.col.size());
            val.resize(val.size() + mat.val.size());
            oneapi::tbb::parallel_for(0, (int) mat.col.size(),
                                      [&](int k) { col[base_col + k] = mat.col[k]; });
            oneapi::tbb::parallel_for(0, (int) mat.val.size(),
                                      [&](int k) { val[base_col + k] = mat.val[k]; });
        }

        [[nodiscard]] std::string toString() const {
            std::string ret;
            for (int i : row) ret += fmt::format("{}, ", i);
            ret += "\n";
            for (int i : col) ret += fmt::format("{}, ", i);
            ret += "\n";
            for (double i : val) ret += fmt::format("{}, ", i);
            ret += "\n";
            for (double i : rhs) ret += fmt::format("{}, ", i);
            ret += "\n";
            return ret;
        }

        std::vector<int> row {0}, col;
        std::vector<Real> val, rhs;
    };
}// namespace OpFlow::DS

#endif//OPFLOW_CSRMATRIX_HPP
