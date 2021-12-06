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

#include "Core/Equation/Equation.hpp"
#include "Core/Field/MeshBased/SemiStructured/CartAMRFieldTrait.hpp"
#include "Core/Field/MeshBased/StencilField.hpp"
#include "Core/Field/MeshBased/StencilFieldTrait.hpp"
#include "Core/Field/MeshBased/Structured/CartesianFieldTrait.hpp"
#include "Core/Macros.hpp"
#include "Core/Meta.hpp"
#include "DataStructures/Index/LinearMapper/MDRangeMapper.hpp"
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
    auto makeEqnSolveHandler(F&& f, T&& t, M&& m, IJSolverParams<S> params = IJSolverParams<S> {}) {
        return AMGCLEqnSolveHandler<F, Meta::RealType<T>, Meta::RealType<M>, Meta::RealType<S>>(
                OP_PERFECT_FOWD(f), OP_PERFECT_FOWD(t), OP_PERFECT_FOWD(m), params);
    }

    template <typename F, CartesianFieldType T, typename M, typename S>
    struct AMGCLEqnSolveHandler<F, T, M, S> {
        AMGCLEqnSolveHandler() = default;
        AMGCLEqnSolveHandler(const F& getter, T& target, const M& mapper, IJSolverParams<S> p)
            : eqn_getter {getter}, target(&target), mapper(mapper), solver(p) {
            init();
        }

        void init() {
            staticMat = solver.getParams().staticMat;
            pinValue = solver.getParams().pinValue;
            auto stField = target->getStencilField();
            stField.pin(pinValue);
            stencilField = std::make_unique<StencilField<T>>(std::move(stField));
            equation = std::make_unique<Eqn>(eqn_getter(*stencilField));
            fieldsAllocated = true;

            auto t = equation->lhs - equation->rhs;
            t.prepare();
            uniEqn = std::make_unique<EqExpr>(std::move(t));
            initStencil();
        }

        void initStencil() {
            // assume the middle stencil is complete
            DS::MDIndex<dim> middle;
            for (auto i = 0; i < dim; ++i) {
                middle[i] = (target->assignableRange.start[i] + target->assignableRange.end[i]) / 2;
            }
            // here we only need the size of the common stencil
            commStencil = uniEqn->evalAt(middle);
        }

        void generateAb() {
            row.clear();
            col.clear();
            val.clear();
            rhs.clear();
            auto local_range = DS::commonRange(target->assignableRange, target->localRange);
            // prepare: evaluate the common stencil & pre-fill the arrays
            int stencil_size = commStencil.pad.size();
            row.resize(local_range.count() + 1);
#pragma omp parallel for default(shared)
            for (int i = 0; i < row.size(); ++i) { row[i] = i * stencil_size; }
            col.resize(local_range.count() * stencil_size);
            val.resize(local_range.count() * stencil_size);
            rhs.resize(local_range.count());
            auto local_mapper = DS::MDRangeMapper<dim> {local_range};
            // first pass: generate the local block of matrix
            rangeFor(local_range, [&](auto&& k) {
                auto currentStencil = uniEqn->evalAt(k);
                int _local_rank = local_mapper(k);
                int _iter = 0;
                for (const auto& [key, v] : currentStencil.pad) {
                    auto idx = mapper(key);
                    col[stencil_size * _local_rank + _iter] = idx;
                    val[stencil_size * _local_rank + _iter] = v;
                    _iter++;
                }
                rhs[_local_rank] = -currentStencil.bias;
                if (_iter < stencil_size) {
                    // boundary case. find the neighbor ranks and assign 0 to them
                    auto local_max = *std::max_element(col.begin() + stencil_size * _local_rank,
                                                       col.begin() + stencil_size * _local_rank + _iter);
                    auto local_min = *std::min_element(col.begin() + stencil_size * _local_rank,
                                                       col.begin() + stencil_size * _local_rank + _iter);
                    if (local_max + stencil_size - _iter <= mapper(target->assignableRange.last())) {
                        // use virtual indexes upper side
                        for (; _iter < stencil_size; ++_iter) {
                            col[stencil_size * _local_rank + _iter] = ++local_max;
                            val[stencil_size * _local_rank + _iter] = 0;
                        }
                    } else if (local_min - (stencil_size - _iter)
                               >= mapper(target->assignableRange.first())) {
                        // use virtual indexes lower side
                        for (; _iter < stencil_size; ++_iter) {
                            col[stencil_size * _local_rank + _iter] = --local_min;
                            val[stencil_size * _local_rank + _iter] = 0;
                        }
                    } else {
                        // the case may be tiny
                        OP_CRITICAL("AMGCL: Cannot find proper filling. Abort.");
                        OP_ABORT;
                    }
                }
            });
            if (pinValue) {
                if (DS::inRange(target->localRange, DS::MDIndex<dim>(target->assignableRange.start))) {
                    for (auto i = 0; i < row[1]; ++i) {
                        val[i] = 0.;
                        if (col[i] == 0) val[i] = 1.;
                    }
                    rhs[0] = 0.;
                }
            }

            if (solver.getParams().dumpPath) {
                std::fstream of;
                of.open(fmt::format("{}A_{}.mat", solver.getParams().dumpPath.value(), getWorkerId()),
                        std::ofstream::out | std::ofstream::ate);
                for (const auto& p : row) of << fmt::format("{} ", p);
                of << std::endl;
                for (const auto& c : col) of << fmt::format("{} ", c);
                of << std::endl;
                for (const auto& v : val) of << fmt::format("{} ", v);
                of << std::endl;
                for (const auto& r : rhs) of << fmt::format("{} ", r);
                of.flush();
            }
        }

        void generateb() {
            auto local_range = DS::commonRange(target->assignableRange, target->localRange);
            auto local_mapper = DS::MDRangeMapper<dim> {local_range};
            rhs.resize(local_range.count());
            rangeFor(local_range, [&](auto&& k) {
                auto currentStencil = uniEqn->evalAt(k);
                rhs[local_mapper(k)] = (-currentStencil.bias);
            });
        }

        void initx() {
            auto local_range = DS::commonRange(target->assignableRange, target->localRange);
            auto local_mapper = DS::MDRangeMapper<dim> {local_range};
            x.resize(local_range.count());
            rangeFor(local_range, [&](auto&& k) { x[local_mapper(k)] = (target->evalAt(k)); });
        }

        void returnValues() {
            auto local_range = DS::commonRange(target->assignableRange, target->localRange);
            auto local_mapper = DS::MDRangeMapper<dim> {local_range};
            rangeFor(local_range, [&](auto&& k) { target->operator[](k) = x[local_mapper(k)]; });
            target->updatePadding();
        }

        void solve() {
            if (firstRun) {
                generateAb();
                initx();
                solver.init(x.size(), row, col, val);
                solver.solve(rhs, x);
                firstRun = false;
            } else {
                if (staticMat) generateb();
                else
                    generateAb();
                initx();
                solver.init(x.size(), row, col, val);
                solver.solve(rhs, x);
            }
            if (solver.getParams().verbose && getWorkerId() == 0) fmt::print("AMGCL: {}\n", solver.logInfo());
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

        constexpr static auto dim = internal::CartesianFieldExprTrait<T>::dim;
    };
}// namespace OpFlow

#endif//OPFLOW_AMGCLEQNSOLVEHANDLER_HPP
