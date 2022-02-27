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

#ifndef OPFLOW_AMGCLEQNSOLVEHANDLER_HPP
#define OPFLOW_AMGCLEQNSOLVEHANDLER_HPP

#include "Core/Equation/AMGCLBackend.hpp"
#include "Core/Equation/CSRMatrixGenerator.hpp"
#include "Core/Equation/EqnSolveHandler.hpp"
#include "Core/Equation/Equation.hpp"
#include "Core/Equation/EquationHolder.hpp"
#include "Core/Equation/StencilHolder.hpp"
#include "Core/Field/MeshBased/SemiStructured/CartAMRFieldTrait.hpp"
#include "Core/Field/MeshBased/StencilField.hpp"
#include "Core/Field/MeshBased/StencilFieldTrait.hpp"
#include "Core/Field/MeshBased/Structured/CartesianFieldTrait.hpp"
#include "Core/Macros.hpp"
#include "Core/Meta.hpp"
#include "DataStructures/Index/LinearMapper/MDRangeMapper.hpp"
#include "DataStructures/Matrix/CSRMatrix.hpp"
#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <oneapi/tbb/concurrent_vector.h>
#include <vector>

#ifdef OPFLOW_WITH_MPI
#include <mpi.h>
#endif

#include "Core/Solvers/IJ/IJSolver.hpp"

namespace OpFlow {

    template <typename F, typename T, typename M, typename S>
    struct AMGCLEqnSolveHandler;
    /*
    template <typename F, CartesianFieldType T, typename M, typename S>
    struct AMGCLEqnSolveHandler<F, T, M, S> : virtual public EqnSolveHandler {
        AMGCLEqnSolveHandler() = default;
        AMGCLEqnSolveHandler(const F& getter, T& target, const M& mapper, IJSolverParams<S> p)
            : eqn_getter {getter}, target(&target), mapper(mapper), solver(p) {
            this->init();
        }
        ~AMGCLEqnSolveHandler() override {
            free(row);
            free(col);
            free(val);
        }

        void init() override {
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
            allocArrays();
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

        void allocArrays() {
            auto local_range = DS::commonRange(target->assignableRange, target->localRange);
            int stencil_size = commStencil.pad.size() * 1.5;
            prev_mat_size = local_range.count() + 1;
            row = reinterpret_cast<decltype(row)>(malloc(sizeof(*row) * prev_mat_size));
#pragma omp parallel for default(shared)
            for (int i = 0; i < prev_mat_size; ++i) { row[i] = i * stencil_size; }
            rhs.resize(prev_mat_size);
            x.resize(prev_mat_size);
            prev_nnz = prev_mat_size * stencil_size;
            col = reinterpret_cast<decltype(col)>(malloc(sizeof(*col) * prev_nnz));
            val = reinterpret_cast<decltype(val)>(malloc(sizeof(*val) * prev_nnz));
            for (int i = 0; i < prev_nnz; ++i) {
                col[i] = std::numeric_limits<std::ptrdiff_t>::quiet_NaN();
                val[i] = std::numeric_limits<Real>::quiet_NaN();
            }
        }

        void generateAb() override {
            auto local_range = DS::commonRange(target->assignableRange, target->localRange);
            // prepare: evaluate the common stencil & pre-fill the arrays
            int stencil_size = commStencil.pad.size() * 1.5;

            auto local_mapper = DS::MDRangeMapper<dim> {local_range};
            int local_offset = pinValue && mapper(local_range.first()) == 0 ? 1 : 0;
            // first pass: generate the local block of matrix
            rangeFor(local_range, [&](auto&& k) {
                // delete the pinned equation
                if (pinValue && mapper(k) == 0) return;
                auto currentStencil = uniEqn->evalAt(k);
                int _local_rank = local_mapper(k) - local_offset;
                int _iter = 0;
                for (const auto& [key, v] : currentStencil.pad) {
                    auto idx = pinValue ? mapper(key) - 1 : mapper(key);
                    col[stencil_size * _local_rank + _iter] = idx;
                    val[stencil_size * _local_rank + _iter] = v;
                    _iter++;
                }
                rhs[_local_rank] = -currentStencil.bias;
                if (_iter < stencil_size) {
                    // boundary case. find the neighbor ranks and assign 0 to them
                    auto local_max = *std::max_element(col + stencil_size * _local_rank,
                                                       col + stencil_size * _local_rank + _iter);
                    auto local_min = *std::min_element(col + stencil_size * _local_rank,
                                                       col + stencil_size * _local_rank + _iter);
                    if (local_max + stencil_size - _iter < mapper(target->assignableRange.last())) {
                        // use virtual indexes upper side
                        for (; _iter < stencil_size; ++_iter) {
                            col[stencil_size * _local_rank + _iter] = ++local_max;
                            val[stencil_size * _local_rank + _iter] = 0;
                        }
                    } else if (local_min - (stencil_size - _iter) >= 0) {
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
                } else if (_iter > stencil_size) {
                    OP_CRITICAL("{} > {}. stencil_size is wrong.", _iter, stencil_size);
                    OP_ABORT;
                }
            });

            if (solver.getParams().dumpPath) {
                std::fstream of;
                of.open(fmt::format("{}A_{}.mat", solver.getParams().dumpPath.value(), getWorkerId()),
                        std::ofstream::out | std::ofstream::ate);
                for (int i = 0; i < local_range.count() + 1; ++i) of << fmt::format("{} ", row[i]);
                of << std::endl;
                for (int i = 0; i < local_range.count() * stencil_size; ++i) of << fmt::format("{} ", col[i]);
                of << std::endl;
                for (int i = 0; i < local_range.count() * stencil_size; ++i) of << fmt::format("{} ", val[i]);
                of << std::endl;
                for (int i = 0; i < local_range.count(); ++i) of << fmt::format("{} ", rhs[i]);
                of << std::endl;
                of.flush();
                for (int i = 0; i < local_range.count(); ++i) {
                    for (int j = row[i]; j < row[i + 1]; ++j) {
                        of << fmt::format("({}, {}) = {}\n", i, col[j], val[j]);
                    }
                    of << fmt::format("rhs[{}] = {}\n", i, rhs[i]);
                }
                of.flush();
                rangeFor_s(local_range, [&](auto&& k) {
                    if (pinValue && mapper(k) == 0) return;
                    auto currentStencil = uniEqn->evalAt(k);
                    of << fmt::format("stencil[{}] = {}\n", k, currentStencil);
                });
                of.flush();
            }
        }

        void generateb() {
            auto local_range = DS::commonRange(target->assignableRange, target->localRange);
            auto local_mapper = DS::MDRangeMapper<dim> {local_range};
            int local_offset = mapper(local_range.first()) == 0 ? 1 : 0;
            if (pinValue)
                rangeFor(local_range, [&](auto&& k) {
                    if (mapper(k) == 0) return;
                    auto currentStencil = uniEqn->evalAt(k);
                    rhs[local_mapper(k) - local_offset] = -currentStencil.bias;
                });
            else
                rangeFor(local_range, [&](auto&& k) {
                    auto currentStencil = uniEqn->evalAt(k);
                    rhs[local_mapper(k)] = (-currentStencil.bias);
                });
        }

        void initx() {
            auto local_range = DS::commonRange(target->assignableRange, target->localRange);
            auto local_mapper = DS::MDRangeMapper<dim> {local_range};
            int local_offset = mapper(local_range.first()) == 0 ? 1 : 0;

            if (pinValue)
                rangeFor(local_range, [&](auto&& k) {
                    if (mapper(k) == 0) return;
                    x[local_mapper(k) - local_offset] = (target->evalAt(k));
                });
            else
                rangeFor(local_range, [&](auto&& k) { x[local_mapper(k)] = (target->evalAt(k)); });
        }

        void returnValues() {
            auto local_range = DS::commonRange(target->assignableRange, target->localRange);
            auto local_mapper = DS::MDRangeMapper<dim> {local_range};
            int local_offset = mapper(local_range.first()) == 0 ? 1 : 0;

            if (pinValue)
                rangeFor(local_range, [&](auto&& k) {
                    target->operator[](k) = mapper(k) == 0 ? 0 : x[local_mapper(k) - local_offset];
                });
            else
                rangeFor(local_range, [&](auto&& k) { target->operator[](k) = x[local_mapper(k)]; });
            target->updatePadding();
        }

        void solve() override {
            if (firstRun) {
                generateAb();
                initx();
                if (pinValue) solver.init(x.size() - 2, row, col, val);
                else
                    solver.init(x.size() - 1, row, col, val);
                solver.solve(rhs, x);
                firstRun = false;
            } else {
                if (staticMat) generateb();
                else
                    generateAb();
                initx();
                if (pinValue) solver.init(x.size() - 2, row, col, val);
                else
                    solver.init(x.size() - 1, row, col, val);
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
        int prev_mat_size = -1, prev_nnz = -1;
        std::ptrdiff_t *row = nullptr, *col = nullptr;
        typename internal::ExprTrait<T>::elem_type* val = nullptr;
        std::vector<typename internal::ExprTrait<T>::elem_type> rhs, x;
        using Stencil
                = DS::StencilPad<DS::ColoredIndex<typename internal::CartesianFieldExprTrait<T>::index_type>>;
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
*/
    template <typename... Fs, typename... Ts, typename M, typename S>
    struct AMGCLEqnSolveHandler<std::tuple<Fs...>, std::tuple<Ts...>, M, S> : virtual public EqnSolveHandler {
    public:
        using eqn_holder_type = decltype(
                makeEqnHolder(std::declval<std::tuple<Fs...>&>(), std::declval<std::tuple<Ts...>&>()));
        using st_holder_type = decltype(makeStencilHolder(std::declval<eqn_holder_type&>()));
        constexpr static int size = sizeof...(Ts);

    private:
        std::unique_ptr<eqn_holder_type> eqn_holder;
        std::unique_ptr<st_holder_type> st_holder;
        std::vector<bool> pin;
        DS::CSRMatrix mat;
        std::vector<IJSolverParams<S>> params;
        M mapper;
        AMGCLBackend<S, Real> solver;
        // temporary data containers
        std::vector<double> x;

        // status
        bool firstRun = true;
        bool staticMat = false;

    public:
        AMGCLEqnSolveHandler() = default;
        AMGCLEqnSolveHandler(eqn_holder_type&& eqn_holder, const M& mapper,
                             const std::vector<IJSolverParams<S>>& params)
            : mapper(mapper), params(params) {
            this->eqn_holder = std::make_unique<eqn_holder_type>(std::move(eqn_holder));
            this->st_holder = std::make_unique<st_holder_type>(*this->eqn_holder);
            init();
        }

        void init() override {
            pin.clear();
            for (const auto& p : params) pin.push_back(p.pinValue);
            staticMat = true;
            for (const auto& p : params) staticMat &= p.staticMat;
        }

        void generateAb() { mat = CSRMatrixGenerator::generate(*st_holder, mapper, pin); }

        void generateb() { CSRMatrixGenerator::generate_rhs(*st_holder, mapper, pin, mat); }

        void initx() {
            x.resize(mat.rhs.size());
            if (firstRun) x.assign(x.size(), 0.);
        }

        void returnValues() {
            Meta::static_for<size>([&]<int i>(Meta::int_<i>) {
                auto target = eqn_holder->template getTargetPtr<i>();
                rangeFor(target->assignableRange, [&](auto&& k) {
                    (*target)[k] = x[mapper(DS::ColoredIndex<Meta::RealType<decltype(k)>> {k, i})];
                });
            });
        }

        void solve() override {
            if (firstRun) {
                generateAb();
                initx();
                if (staticMat) solver.solve_dy(mat, x, params[0].p, params[0].bp, params[0].verbose);
                else
                    AMGCLBackend<S, Real>::solve(mat, x, params[0].p, params[0].bp, params[0].verbose);
                firstRun = false;
            } else {
                initx();
                if (staticMat) {
                    generateb();
                    solver.solve_dy(mat, x, params[0].p, params[0].bp, params[0].verbose);
                } else {
                    generateAb();
                    AMGCLBackend<S, Real>::solve(mat, x, params[0].p, params[0].bp, params[0].verbose);
                }
            }
            returnValues();
        }
    };

    template <typename S, typename F, typename T, typename M>
    std::unique_ptr<EqnSolveHandler> makeEqnSolveHandler(F&& f, T&& t, M&& mapper,
                                                         IJSolverParams<S> params = IJSolverParams<S> {}) {
        auto getters = std::forward_as_tuple(OP_PERFECT_FOWD(f));
        auto targets = std::forward_as_tuple(OP_PERFECT_FOWD(t));

        return std::make_unique<
                AMGCLEqnSolveHandler<Meta::RealType<decltype(getters)>, Meta::RealType<decltype(targets)>,
                                     Meta::RealType<M>, Meta::RealType<S>>>(makeEqnHolder(getters, targets),
                                                                            mapper, std::vector {params});
    }

    template <typename S>
    std::unique_ptr<EqnSolveHandler> makeEqnSolveHandler(auto&&... fs) requires(sizeof...(fs) >= 6) {
        auto t = std::forward_as_tuple(OP_PERFECT_FOWD(fs)...);
        auto&& [getters, rest1] = Meta::tuple_split<sizeof...(fs) / 2 - 1>(t);
        auto&& [targets, rest2] = Meta::tuple_split<sizeof...(fs) / 2 - 1>(rest1);
        auto&& [mapper, params] = rest2;
        return std::make_unique<
                AMGCLEqnSolveHandler<Meta::RealType<decltype(getters)>, Meta::RealType<decltype(targets)>,
                                     Meta::RealType<decltype(mapper)>, S>>(makeEqnHolder(getters, targets),
                                                                           mapper, params);
    }
}// namespace OpFlow

#endif//OPFLOW_AMGCLEQNSOLVEHANDLER_HPP
