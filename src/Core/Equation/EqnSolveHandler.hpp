// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2021 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_EQNSOLVEHANDLER_HPP
#define OPFLOW_EQNSOLVEHANDLER_HPP

#include "Core/Equation/Equation.hpp"
#include "Core/Field/MeshBased/StencilField.hpp"
#include "Core/Field/MeshBased/StencilFieldTrait.hpp"
#include "Core/Field/MeshBased/Structured/CartesianField.hpp"
#include "Core/Field/MeshBased/Structured/CartesianFieldExprTrait.hpp"
#include "Core/Field/MeshBased/Structured/CartesianFieldTrait.hpp"
#include "Core/Solvers/SemiStruct/SemiStructSolver.hpp"
#include "Core/Solvers/Struct/StructSolver.hpp"
#include "DataStructures/Index/MDIndex.hpp"
#include <memory>
#include <numeric>
#include <vector>

#include <HYPRE.h>

namespace OpFlow {
    template <typename F, typename T, typename S>
    struct EqnSolveHandler;

    template <typename F, typename T, typename S>
    auto makeEqnSolveHandler(F&& f, T&& t, S&& s) {
        return EqnSolveHandler<F, Meta::RealType<T>, Meta::RealType<S>>(
                OP_PERFECT_FOWD(f), OP_PERFECT_FOWD(t), OP_PERFECT_FOWD(s));
    }

    template <typename F, CartesianFieldType T, typename Solver>
    struct EqnSolveHandler<F, T, Solver> {
        EqnSolveHandler() = default;
        EqnSolveHandler(const F& getter, T& target, const Solver& s)
            : getters {getter}, targets {&target}, solver(s) {
            equations.reserve(1);
            stencilFields.reserve(1);
            uniEqs.reserve(1);
            init();
        }
        EqnSolveHandler(const std::vector<F>& getter, const std::vector<std::add_pointer_t<T>>& target,
                        const Solver& s)
            : getters(getter), targets(target), solver(s) {
            equations.reserve(getter.size());
            stencilFields.reserve(getter.size());
            uniEqs.reserve(getter.size());
            init();
        }
        EqnSolveHandler(std::vector<F>&& getter, std::vector<std::add_pointer_t<T>>&& target, const Solver& s)
            : getters(std::move(getter)), targets(std::move(target)), solver(s) {
            equations.reserve(getters.size());
            stencilFields.reserve(getters.size());
            uniEqs.reserve(getters.size());
            init();
        }
        ~EqnSolveHandler() {
            HYPRE_StructMatrixDestroy(A);
            HYPRE_StructVectorDestroy(b);
            HYPRE_StructVectorDestroy(x);
            HYPRE_StructGridDestroy(grid);
            HYPRE_StructStencilDestroy(stencil);
        }

        void init() {
            for (auto i = 0; i < targets.size(); ++i) {
                auto stField = targets[i]->getStencilField();
                if (i == 0) stField.pin(solver.params.pinValue);
                stencilFields.push_back(std::move(stField));
                equations.push_back(getters[i](stencilFields.back()));
            }
            fieldsAllocated = true;

            initStencil();
            initAbx();
            solver.init();
        }

        void initStencil() {
            HYPRE_StructGridCreate(solver.params.comm, dim, &grid);
            for (auto& e : equations) {
                auto t = e.lhs - e.rhs;
                t.prepare();
                uniEqs.push_back(t);
            }
            for (auto i = 0; i < uniEqs.size(); ++i) {
                auto& t = *targets[i];
                auto r = t.assignableRange.end;
                for (auto j = 0; j < r.size(); ++j) r[j] -= 1;
                HYPRE_StructGridSetExtents(grid, t.assignableRange.start.data(), r.data());
            }
            HYPRE_StructGridAssemble(grid);

            // assume the middle stencil is complete
            DS::MDIndex<dim> middle;
            for (auto i = 0; i < dim; ++i)
                middle[i] = (targets[0]->assignableRange.start[i] + targets[0]->assignableRange.end[i]) / 2;
            commStencil = getOffsetStencil(uniEqs[0].evalSafeAt(middle), middle);
            HYPRE_StructStencilCreate(dim, commStencil.pad.size(), &stencil);
            auto iter = commStencil.pad.begin();
            for (auto i = 0; i < commStencil.pad.size(); ++i, ++iter) {
                HYPRE_StructStencilSetElement(stencil, i, const_cast<int*>(iter->first.get().data()));
            }
        }

        void initAbx() {
            HYPRE_StructMatrixCreate(solver.params.comm, grid, stencil, &A);
            HYPRE_StructMatrixInitialize(A);
            HYPRE_StructVectorCreate(solver.params.comm, grid, &b);
            HYPRE_StructVectorCreate(solver.params.comm, grid, &x);
            HYPRE_StructVectorInitialize(b);
            HYPRE_StructVectorInitialize(x);
        }

        void initx() {
            for (auto i = 0; i < targets.size(); ++i) {
                structFor(*targets[i], targets[i]->assignableRange, [&](auto&& k) {
                    HYPRE_StructVectorSetValues(x, const_cast<int*>(k.get().data()), targets[i]->evalAt(k));
                });
            }
        }

        void generateAb() {
            std::vector<int> entries(commStencil.pad.size());
            for (auto i = 0; i < entries.size(); ++i) entries[i] = i;

            for (auto i = 0; i < targets.size(); ++i) {
                structFor(uniEqs[i], targets[i]->assignableRange, [&](auto&& k) {
                    auto currentStencil = getOffsetStencil(uniEqs[i].evalSafeAt(k), k);
                    auto extendedStencil = commonStencil(currentStencil, commStencil);
                    std::vector<Real> vals;
                    for (const auto& [key, val] : commStencil.pad) {
                        vals.push_back(extendedStencil.pad[key]);
                    }
                    HYPRE_StructMatrixSetValues(A, const_cast<int*>(k.get().data()), commStencil.pad.size(),
                                                entries.data(), vals.data());
                    HYPRE_StructVectorSetValues(b, const_cast<int*>(k.get().data()), -extendedStencil.bias);
                });
            }

            if (solver.params.pinValue) {
                // pin the first unknown to 0
                auto identical = DS::StencilPad<DS::MDIndex<dim>>();
                auto first = DS::MDIndex<dim>(targets[0]->assignableRange.start);
                identical.pad[first] = 1.0;
                identical.bias = 0.;
                auto extendedStencil = commonStencil(identical, commStencil);
                std::vector<Real> vals;
                for (const auto& [key, val] : commStencil.pad) { vals.push_back(extendedStencil.pad[key]); }
                HYPRE_StructMatrixSetValues(A, const_cast<int*>(first.get().data()), commStencil.pad.size(),
                                            entries.data(), vals.data());
                HYPRE_StructVectorSetValues(b, const_cast<int*>(first.get().data()), -extendedStencil.bias);
            }
            HYPRE_StructMatrixAssemble(A);
            HYPRE_StructVectorAssemble(b);
        }

        void generateb() {
            for (auto i = 0; i < targets.size(); ++i) {
                structFor(uniEqs[i], targets[i]->assignableRange, [&](auto&& k) {
                    auto currentStencil = uniEqs[i].evalSafeAt(k);
                    HYPRE_StructVectorSetValues(b, const_cast<int*>(k.get().data()), -currentStencil.bias);
                });
            }
            if (solver.params.pinValue) {
                auto first = DS::MDIndex<dim>(targets[0]->assignableRange.start);
                HYPRE_StructVectorSetValues(b, const_cast<int*>(first.get().data()), 0.);
            }
            HYPRE_StructVectorAssemble(b);
        }

        void returnValues() {
            for (auto i = 0; i < targets.size(); ++i) {
                structFor(*targets[i], targets[i]->assignableRange, [&](auto&& k) {
                    Real val;
                    HYPRE_StructVectorGetValues(x, const_cast<int*>(k.get().data()), &val);
                    targets[i]->operator[](k) = val;
                });
            }
        }

        void solve() {
            if (firstRun) {
                generateAb();
                initx();
                solver.getSolver().dump(A, b);
                solver.setup(A, b, x);
                solver.solve(A, b, x);
                firstRun = false;
            } else {
                if (solver.params.staticMat) generateb();
                else
                    generateAb();
                initx();
                solver.solve(A, b, x);
            }
            returnValues();
        }

        std::vector<F> getters;
        std::vector<std::add_pointer_t<T>> targets;
        using Stencil = DS::StencilPad<typename internal::CartesianFieldExprTrait<T>::index_type>;
        using Eqn = Meta::RealType<decltype(std::declval<F>()(std::declval<StencilField<T>&>()))>;
        std::vector<Eqn> equations;
        using EqExpr = Meta::RealType<decltype(equations[0].lhs - equations[0].rhs)>;
        std::vector<EqExpr> uniEqs;
        Stencil commStencil;
        std::vector<StencilField<T>> stencilFields;
        bool fieldsAllocated = false;
        bool firstRun = true;
        Solver solver;
        HYPRE_StructStencil stencil {};
        HYPRE_StructGrid grid {};
        HYPRE_StructMatrix A {};
        HYPRE_StructVector b {}, x {};

    private:
        constexpr static auto dim = internal::CartesianFieldExprTrait<T>::dim;
    };
}// namespace OpFlow
#endif//OPFLOW_EQNSOLVEHANDLER_HPP
