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

        void generateAb() override { mat = CSRMatrixGenerator::generate(*st_holder, mapper, pin); }

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
                target->updatePadding();
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
