//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2023 by the OpFlow developers
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
#include "DataStructures/Arrays/Arrays.hpp"
#include <fmt/format.h>
#include <oneapi/tbb/parallel_for.h>

namespace OpFlow::DS {
    struct CSRMatrix {
        CSRMatrix() : row(1), col(0), rhs(0), val(0) { row[0] = 0; }
        CSRMatrix(int n_row, int nnz_per_row) { resize(n_row, nnz_per_row); }

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

        void append_s(const CSRMatrix& mat) {
            int base_row = row.size(), base_rhs = rhs.size(), offset = row.back();
            row.resize(row.size() + mat.row.size() - 1);
            rhs.resize(rhs.size() + mat.rhs.size());
            for (int i = 0; i < mat.row.size() - 1; ++i) row[base_row + i] = mat.row[i + 1] + offset;
            for (int i = 0; i < mat.rhs.size(); ++i) rhs[base_rhs + i] = mat.rhs[i];
            int base_col = col.size();
            col.resize(col.size() + mat.col.size());
            val.resize(val.size() + mat.val.size());
            for (int i = 0; i < mat.col.size(); ++i) col[base_col + i] = mat.col[i];
            for (int i = 0; i < mat.val.size(); ++i) val[base_col + i] = mat.val[i];
        }

        void update_rhs(int from, const std::vector<Real>& r) {
            std::copy(r.begin(), r.end(), rhs.begin() + from);
        }

        [[nodiscard]] std::string toString() const {
            std::string ret;
            int max_nnz_per_row = 0;
            int max_rank_width = 0, max_col_width = 0, max_val_width = 0;
            for (int i = 0; i < row.size() - 1; ++i) {
                max_nnz_per_row = std::max(max_nnz_per_row, int(row[i + 1] - row[i]));
            }
            for (auto r : row) {
                max_rank_width = std::max(max_rank_width, (int) fmt::formatted_size("{}", r));
            }
            for (auto c : col) {
                max_col_width = std::max(max_col_width, (int) fmt::formatted_size("{}", c));
            }
            for (auto v : val) {
                max_val_width = std::max(max_val_width, (int) fmt::formatted_size("{}", v));
            }
            max_val_width = std::min(max_val_width, 10);
            for (int irow = 0; irow < row.size() - 1; ++irow) {
                ret += fmt::format("row {:>{}}: [{:>{}}, {:> {}.4E}] ", irow, max_rank_width, col[row[irow]],
                                   max_col_width, val[row[irow]], max_val_width);
                for (int icol = row[irow] + 1; icol < row[irow + 1]; ++icol) {
                    ret += fmt::format("[{:>{}}, {:> {}.4E}] ", col[icol], max_col_width, val[icol],
                                       max_val_width);
                }
                for (int icol = row[irow + 1]; icol < row[irow] + max_nnz_per_row; ++icol) {
                    ret += std::string(fmt::formatted_size("[{:>{}}, {:> {}.4E}] ", 0, max_col_width, 1.0,
                                                           max_val_width),
                                       ' ');
                }
                ret += fmt::format("rhs: {:> {}.4E}\n", rhs[irow], max_val_width);
            }
            return ret;
        }

        // print in dense format
        [[nodiscard]] std::string toString(bool force) const {
            if (row.size() > 129 && !force) {
                // we consider a matrix too large if #row > 128
                OP_WARN("CSRMatrix: the matrix to be serialized too large (> 128 rows). Fall back to compact "
                        "mode");
                return toString();
            } else {
                int mat_size = (int) row.size() - 1;
                DS::PlainTensor<Real, 2> mat(mat_size, mat_size);
                mat.setZero();
                for (int irow = 0; irow < row.size() - 1; ++irow) {
                    for (int icol = (int) row[irow]; icol < row[irow + 1]; ++icol) {
                        mat((int) irow, (int) col[icol]) = val[icol];
                    }
                }
                std::string ret = "";
                for (int irow = 0; irow < mat_size; ++irow) {
                    for (int icol = 0; icol < mat_size; ++icol) {
                        ret += fmt::format("{:>6.2f} ", mat(irow, icol));
                    }
                    ret += fmt::format("rhs[{}] = {:>6.2f}\n", irow, rhs[irow]);
                }
                return ret;
            }
        }

        DS::DenseVector<std::ptrdiff_t> row, col;
        DS::DenseVector<Real> val;
        std::vector<Real> rhs;// AMGCL requirement
    };
}// namespace OpFlow::DS

#endif//OPFLOW_CSRMATRIX_HPP
