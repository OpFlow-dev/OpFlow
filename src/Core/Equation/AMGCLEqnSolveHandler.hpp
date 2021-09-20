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

#ifndef OPFLOW_AMGCLEQNSOLVEHANDLER_HPP
#define OPFLOW_AMGCLEQNSOLVEHANDLER_HPP

#include "Core/Field/MeshBased/SemiStructured/CartAMRFieldTrait.hpp"
#include "Core/Field/MeshBased/Structured/CartesianFieldTrait.hpp"
#include "Core/Macros.hpp"
#include "Core/Meta.hpp"
#include <cstddef>
#include <fstream>
#include <vector>

#ifdef OPFLOW_WITH_MPI
#include <mpi.h>
#endif

#include "Core/Solvers/IJ/IJSolver.hpp"

namespace OpFlow {
    template <typename F, typename T, typename M, typename S>
    struct AMGCLEqnSolveHandler;

    template <typename S, typename F, typename T, typename M>
    auto makeEqnSolveHandler(F&& f, T&& t, M&& m,
                             typename Meta::RealType<S>::params p = typename Meta::RealType<S>::params {},
                             typename Meta::RealType<S>::backend_params bp
                             = typename Meta::RealType<S>::backend_params {}) {
        return AMGCLEqnSolveHandler<F, Meta::RealType<T>, Meta::RealType<M>, Meta::RealType<S>>(
                OP_PERFECT_FOWD(f), OP_PERFECT_FOWD(t), OP_PERFECT_FOWD(m), p, bp);
    }

    template <typename F, CartesianFieldType T, typename M, typename S>
    struct AMGCLEqnSolveHandler<F, T, M, S> {
        AMGCLEqnSolveHandler() = default;
        AMGCLEqnSolveHandler(const F& getter, T& target, const M& mapper,
                             typename Meta::RealType<S>::params p,
                             typename Meta::RealType<S>::backend_params bp)
            : eqn_getter {getter}, target(&target), mapper(mapper), solver(p, bp) {
            init();
        }

        void init() {
            auto stField = target->getStencilField();
            stField.pin(pinValue);
            stencilField = std::make_unique<StencilField<T>>(std::move(stField));
            equation = std::make_unique<Eqn>(eqn_getter(*stencilField));
            fieldsAllocated = true;

            auto t = equation->lhs - equation->rhs;
            t.prepare();
            uniEqn = std::make_unique<EqExpr>(std::move(t));
        }

        void generateAb() {
            row.clear();
            col.clear();
            val.clear();
            rhs.clear();
            std::vector<int> nnz_counts(getGlobalParallelPlan().distributed_workers_count, 0);
            // first pass: generate the local block of matrix
            int count = 0;
            row.push_back(count);
            rangeFor_s(DS::commonRange(target->assignableRange, target->localRange), [&](auto&& k) {
                auto currentStencil = uniEqn->evalSafeAt(k);
                for (const auto& [key, v] : currentStencil.pad) {
                    auto idx = mapper(key);
                    col.push_back(idx);
                    val.push_back(v);
                    count++;
                }
                rhs.push_back(-currentStencil.bias);
                row.push_back(count);
            });
            nnz_counts[getWorkerId()] = count;
            // communicate within group the nnz_counts
#if defined(OPFLOW_WITH_MPI) && defined(OPFLOW_DISTRIBUTE_MODEL_MPI)
            MPI_Allgather(&count, 1, MPI_INT, nnz_counts.data(), 1, MPI_INT, MPI_COMM_WORLD);
            if (getWorkerId() == 1) {
                for (auto i = 0; i < nnz_counts.size(); ++i) OP_INFO("Rank {} count = {}", i, nnz_counts[i]);
            }
#endif
            // second pass: add the row offset to the row array
            if (false && getWorkerId() > 0) {
                int offset = 0;
                for (auto i = 0; i < getWorkerId(); ++i) offset += nnz_counts[i];
                OP_INFO("RANK {} OFFSET = {}", getWorkerId(), offset);
                for (auto& i : row) i += offset;
            }
            std::fstream of;
            of.open(fmt::format("A_{}.mat", getWorkerId()), std::ofstream::out | std::ofstream::ate);
            //auto& of = std::cout;
            for (const auto& p : row) of << fmt::format("{} ", p);
            of << std::endl;
            for (const auto& c : col) of << fmt::format("{} ", c);
            of << std::endl;
            for (const auto& v : val) of << fmt::format("{} ", v);
            of << std::endl;
            for (const auto& r : rhs) of << fmt::format("{} ", r);
            of.flush();
            //of.close();
            OP_INFO("Generate AB");
        }

        void generateb() {
            rhs.clear();
            rangeFor_s(DS::commonRange(target->assignableRange, target->localRange), [&](auto&& k) {
                auto currentStencil = uniEqn->evalSafeAt(k);
                rhs.push_back(-currentStencil.bias);
            });
        }

        void initx() {
            x.clear();
            rangeFor_s(DS::commonRange(target->assignableRange, target->localRange),
                       [&](auto&& k) { x.push_back(target->evalAt(k)); });
        }

        void returnValues() {
            auto iter = x.begin();
            rangeFor_s(DS::commonRange(target->assignableRange, target->localRange),
                       [&](auto&& k) { target->operator[](k) = *iter++; });
            target->updatePadding();
        }

        void solve() {
            if (firstRun) {
                generateAb();
                initx();
                OP_INFO("INIT0");
                solver.init(x.size(), row, col, val);
                OP_INFO("SOLVE0");
                solver.solve(rhs, x);
                OP_INFO("SOLVE1");
                firstRun = false;
            } else {
                if (staticMat) generateb();
                else
                    generateAb();
                initx();
                solver.init(x.size(), row, col, val);
                solver.solve(rhs, x);
            }
#ifndef NDEBUG
            OP_INFO("AMGCL: {}", solver.logInfo());
#endif
            returnValues();
        }

    private:
        F eqn_getter;
        std::add_pointer_t<T> target;
        M mapper;
        IJSolver<S> solver;
        std::vector<std::ptrdiff_t> row, col;
        std::vector<typename internal::ExprTrait<T>::elem_type> val, rhs, x;
        using Stencil = DS::StencilPad<typename internal::CartesianFieldExprTrait<T>::index_type>;
        using Eqn = Meta::RealType<decltype(std::declval<F>()(std::declval<StencilField<T>&>()))>;
        std::unique_ptr<Eqn> equation;
        using EqExpr = Meta::RealType<decltype(equation->lhs - equation->rhs)>;
        std::unique_ptr<EqExpr> uniEqn;
        Stencil commStencil;
        std::unique_ptr<StencilField<T>> stencilField;
        bool fieldsAllocated = false;
        bool firstRun = true;
        bool staticMat = false, pinValue = false;
    };
}// namespace OpFlow

#endif//OPFLOW_AMGCLEQNSOLVEHANDLER_HPP
