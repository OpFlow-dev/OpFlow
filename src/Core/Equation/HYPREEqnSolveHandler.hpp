// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2026 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_HYPREEQNSOLVEHANDLER_HPP
#define OPFLOW_HYPREEQNSOLVEHANDLER_HPP

#include "Core/Equation/EqnSolveHandler.hpp"
#include "Core/Equation/Equation.hpp"
#include "Core/Field/MeshBased/SemiStructured/CartAMRField.hpp"
#include "Core/Field/MeshBased/SemiStructured/CartAMRFieldExprTrait.hpp"
#include "Core/Field/MeshBased/SemiStructured/CartAMRFieldTrait.hpp"
#include "Core/Field/MeshBased/StencilField.hpp"
#include "Core/Field/MeshBased/StencilFieldTrait.hpp"
#include "Core/Field/MeshBased/Structured/CartesianField.hpp"
#include "Core/Field/MeshBased/Structured/CartesianFieldExprTrait.hpp"
#include "Core/Field/MeshBased/Structured/CartesianFieldTrait.hpp"
#include "Core/Solvers/SemiStruct/SemiStructSolver.hpp"
#include "Core/Solvers/Struct/StructSolver.hpp"
#include "DataStructures/Index/LevelMDIndex.hpp"
#include "DataStructures/Index/MDIndex.hpp"
#ifndef OPFLOW_INSIDE_MODULE
#include <format>
#include <iostream>
#include <memory>
#include <numeric>
#include <vector>

#include <HYPRE.h>
#include <HYPRE_sstruct_ls.h>
#include <HYPRE_sstruct_mv.h>
#endif

OPFLOW_MODULE_EXPORT

namespace OpFlow {
    template <typename F, typename T, typename S>
    struct HYPREEqnSolveHandler;

    template <typename F, typename T, typename S>
    std::unique_ptr<EqnSolveHandler> makeEqnSolveHandler(F&& f, T&& t, S&& s) {
        return std::make_unique<HYPREEqnSolveHandler<F, Meta::RealType<T>, Meta::RealType<S>>>(
                OP_PERFECT_FOWD(f), OP_PERFECT_FOWD(t), OP_PERFECT_FOWD(s));
    }

    template <typename F, CartesianFieldType T, typename Solver>
    struct HYPREEqnSolveHandler<F, T, Solver> : virtual public EqnSolveHandler {
        HYPREEqnSolveHandler() = default;

        HYPREEqnSolveHandler(const F& getter, T& target, const Solver& s)
            : eqn_getter {getter}, target {&target}, solver(s) {
            this->init();
        }

        ~HYPREEqnSolveHandler() override {
            HYPRE_StructMatrixDestroy(A);
            HYPRE_StructVectorDestroy(b);
            HYPRE_StructVectorDestroy(x);
            HYPRE_StructGridDestroy(grid);
            HYPRE_StructStencilDestroy(stencil);
        }

        void init() override {
            auto stField = target->getStencilField();
            stField.pin(solver.params.pinValue);
            stencilField = std::make_unique<StencilField<T>>(std::move(stField));
            stencilField->ignorePeriodicBC();
            equation = std::make_unique<Eqn>(eqn_getter(*stencilField));
            fieldsAllocated = true;

            initStencil();
            initAbx();
            solver.init();
        }

        void initStencil() {
            HYPRE_StructGridCreate(solver.params.comm, dim, &grid);
            auto t = equation->lhs - equation->rhs;
            t.prepare();
            uniEqn = std::make_unique<EqExpr>(std::move(t));
            auto local_assignable_range = DS::commonRange(target->localRange, target->assignableRange);
            auto r = local_assignable_range.end;
            for (std::size_t j = 0; j < r.size(); ++j) r[j] -= 1;
            HYPRE_StructGridSetExtents(grid, local_assignable_range.start.data(), r.data());
            int periodic[dim];
            for (auto j = 0; j < dim; ++j)
                if (target->bc[j].start && target->bc[j].start->getBCType() == BCType::Periodic)
                    periodic[j] = target->assignableRange.end[j] - target->assignableRange.start[j];
                else
                    periodic[j] = 0;
            HYPRE_StructGridSetPeriodic(grid, periodic);
            HYPRE_StructGridAssemble(grid);

            // assume the middle stencil is complete
            // fixme: this is dangerous especially for MPI cases. consider a better plan
            DS::MDIndex<dim> middle;
            for (auto i = 0; i < dim; ++i)
                middle[i] = (target->assignableRange.start[i] + target->assignableRange.end[i]) / 2;
            commStencil = getOffsetStencil(uniEqn->evalAt(middle), middle);
            HYPRE_StructStencilCreate(dim, commStencil.pad.size(), &stencil);
            auto iter = commStencil.pad.begin();
            for (int i = 0; i < commStencil.pad.size(); ++i, ++iter) {
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
            rangeFor(DS::commonRange(target->assignableRange, target->localRange), [&](auto&& k) {
                HYPRE_StructVectorSetValues(x, const_cast<int*>(k.get().data()), target->evalAt(k));
            });
        }

        void generateAb() override {
            std::vector<int> entries(commStencil.pad.size());
            for (std::size_t i = 0; i < entries.size(); ++i) entries[i] = i;

            rangeFor(DS::commonRange(target->assignableRange, target->localRange), [&](auto&& k) {
                auto currentStencil = getOffsetStencil(uniEqn->evalAt(k), k);
                auto extendedStencil = commonStencil(currentStencil, commStencil);
                std::vector<Real> vals;
                for (const auto& [key, val] : commStencil.pad) { vals.push_back(extendedStencil.pad[key]); }
                HYPRE_StructMatrixSetValues(A, const_cast<int*>(k.get().data()), commStencil.pad.size(),
                                            entries.data(), vals.data());
                HYPRE_StructVectorSetValues(b, const_cast<int*>(k.get().data()), -extendedStencil.bias);
            });

            if (solver.params.pinValue) {
                // pin the first unknown to 0
                auto identical = DS::StencilPad<DS::ColoredIndex<DS::MDIndex<dim>>>();
                auto first = DS::ColoredIndex<DS::MDIndex<dim>> {
                        DS::MDIndex<dim> {target->assignableRange.start}};
                if (DS::inRange(target->localRange, first)) {
                    identical.pad[first] = 1.0;
                    identical.bias = 0.;
                    auto extendedStencil = commonStencil(identical, commStencil);
                    std::vector<Real> vals;
                    for (const auto& [key, val] : commStencil.pad) {
                        vals.push_back(extendedStencil.pad[key]);
                    }
                    HYPRE_StructMatrixSetValues(A, const_cast<int*>(first.get().data()),
                                                commStencil.pad.size(), entries.data(), vals.data());
                    HYPRE_StructVectorSetValues(b, const_cast<int*>(first.get().data()),
                                                -extendedStencil.bias);
                }
            }
            HYPRE_StructMatrixAssemble(A);
            HYPRE_StructVectorAssemble(b);
        }

        void generateb() {
            rangeFor(DS::commonRange(target->assignableRange, target->localRange), [&](auto&& k) {
                auto currentStencil = uniEqn->evalAt(k);
                HYPRE_StructVectorSetValues(b, const_cast<int*>(k.get().data()), -currentStencil.bias);
            });
            if (solver.params.pinValue) {
                auto first = DS::MDIndex<dim>(
                        DS::commonRange(target->assignableRange, target->localRange).start);
                HYPRE_StructVectorSetValues(b, const_cast<int*>(first.get().data()), 0.);
            }
            HYPRE_StructVectorAssemble(b);
        }

        void returnValues() {
            rangeFor(DS::commonRange(target->assignableRange, target->localRange), [&](auto&& k) {
                Real val;
                HYPRE_StructVectorGetValues(x, const_cast<int*>(k.get().data()), &val);
                target->operator[](k) = val;
            });
            target->updatePadding();
        }

        EqnSolveState solve() override {
            if (firstRun) {
                generateAb();
                initx();
                solver.dump(A, b);
                solver.setup(A, b, x);
                solver.solve(A, b, x);
                firstRun = false;
            } else {
                if (solver.params.staticMat) generateb();
                else
                    generateAb();
                initx();
                solver.reinit();
                solver.setup(A, b, x);
                solver.solve(A, b, x);
            }
            returnValues();
            return EqnSolveState(solver.getIterNum(), solver.getFinalRes());
        }

        F eqn_getter;
        std::add_pointer_t<T> target;
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
        Solver solver;
        HYPRE_StructStencil stencil {};
        HYPRE_StructGrid grid {};
        HYPRE_StructMatrix A {};
        HYPRE_StructVector b {}, x {};

    private:
        constexpr static auto dim = internal::CartesianFieldExprTrait<T>::dim;
    };

    template <typename F, CartAMRFieldType T, typename Solver>
    struct HYPREEqnSolveHandler<F, T, Solver> : virtual public EqnSolveHandler {
        HYPREEqnSolveHandler() = default;

        HYPREEqnSolveHandler(const F& getter, T& target, const Solver& s)
            : getter(getter), target(&target), solver(s) {
            this->init();
        }

        ~HYPREEqnSolveHandler() override { deallocHYPRE(); }

        void init() override {
            stencilField = std::make_unique<StencilField<T>>(target->getStencilField());
            stencilField->pin(solver.params.pinValue);
            equation = std::make_unique<Eqn>(getter(*stencilField));
            uniEqn = std::make_unique<EqExpr>(equation->lhs - equation->rhs);
            fieldsAllocated = true;
            solver.init();
        }

        void allocHYPRE() {
            // grid
            int ndim = dim, nparts = target->getLevels(), nvars = 1;
            int vartypes[] = {HYPRE_SSTRUCT_VARIABLE_CELL};
            HYPRE_SStructGridCreate(solver.params.comm, ndim, nparts, &grid);
            for (std::size_t l = 0; l < target->localRanges.size(); ++l) {
                for (std::size_t p = 0; p < target->localRanges[l].size(); ++p) {
                    auto _local_range = target->localRanges[l][p];
                    for (std::size_t i = 0; i < static_cast<std::size_t>(dim); ++i) _local_range.end[i]--;
                    HYPRE_SStructGridSetExtents(grid, l, _local_range.start.data(), _local_range.end.data());
                }
                HYPRE_SStructGridSetVariables(grid, l, nvars, vartypes);
            }
            HYPRE_SStructGridAssemble(grid);
            // stencil & graph
            DS::ColoredIndex<DS::LevelMDIndex<dim>> middle;
            for (std::size_t i = 0; i < static_cast<std::size_t>(dim); ++i)
                middle[i] = (target->assignableRanges[0][0].start[i] + target->assignableRanges[0][0].end[i])
                            / 2;
            //commStencil = getOffsetStencil(uniEqn->evalAt(middle), middle);
            DS::StencilPad<DS::ColoredIndex<index_type>> _st;
            _st.pad[middle] = 0;
            _st.pad[middle.template next<0>()] = 0;
            _st.pad[middle.template prev<0>()] = 0;
            _st.pad[middle.template next<1>()] = 0;
            _st.pad[middle.template prev<1>()] = 0;
            commStencil = getOffsetStencil(_st, middle);
            HYPRE_SStructStencilCreate(ndim, commStencil.pad.size(), &stencil);
            auto iter = commStencil.pad.begin();
            int c_var = 0;
            for (int i = 0; i < commStencil.pad.size(); ++i, ++iter) {
                HYPRE_SStructStencilSetEntry(stencil, i, const_cast<int*>(iter->first.get().data()), c_var);
            }
            HYPRE_SStructGraphCreate(solver.params.comm, grid, &graph);
            for (std::size_t l = 0; l < target->localRanges.size(); ++l) {
                HYPRE_SStructGraphSetStencil(graph, l, c_var, stencil);
            }
            // inter-level graph entries
            for (int l = 0; l < target->getLevels(); ++l) {
                for (std::size_t p = 0; p < target->localRanges[l].size(); ++p) {
                    rangeFor_s(target->localRanges[l][p], [&](auto&& i) {
                        if (stencilField->blocked(i)) return;
                        auto st = uniEqn->evalAt(i);
                        for (auto& [k, v] : st.pad) {
                            if (k.l != l) {
                                HYPRE_SStructGraphAddEntries(graph, l, i.c_arr(), c_var, k.l, k.c_arr(),
                                                             c_var);
                                std::cout << std::format("GraphAddEntry: {} -> {}\n", i, k);
                            }
                        }
                    });
                }
            }
            HYPRE_SStructGraphAssemble(graph);
            // matrix & rhs
            HYPRE_SStructMatrixCreate(solver.params.comm, graph, &A);
            HYPRE_SStructMatrixInitialize(A);
            HYPRE_SStructVectorCreate(solver.params.comm, grid, &b);
            HYPRE_SStructVectorCreate(solver.params.comm, grid, &x);
            HYPRE_SStructVectorInitialize(b);
            HYPRE_SStructVectorInitialize(x);
            allocated = true;
        }

        void deallocHYPRE() {
            if (!allocated) return;
            HYPRE_SStructMatrixDestroy(A);
            HYPRE_SStructVectorDestroy(b);
            HYPRE_SStructVectorDestroy(x);
            HYPRE_SStructGridDestroy(grid);
            HYPRE_SStructStencilDestroy(stencil);
            HYPRE_SStructGraphDestroy(graph);
            allocated = false;
        }

        void generateAb() override {
            for (int l = 0; l < target->getLevels(); ++l) {
                for (std::size_t p = 0; p < target->localRanges[l].size(); ++p) {
                    rangeFor_s(target->localRanges[l][p], [&](auto&& i) {
                        if (stencilField->blocked(i)) return;
                        // stencil part
                        auto currentStencil = uniEqn->evalAt(i);
                        auto offsetStencil = currentStencil;
                        for (auto& [k, v] : offsetStencil.pad) {
                            if (k.l == l) { k -= i; }
                        }
                        std::cout << std::format("index = {}\n", i);
                        std::cout << std::format("current stencil:{}\noffset stencil:{}\n", currentStencil,
                                                 offsetStencil);
                        auto extendedStencil = offsetStencil;
                        for (auto& [k, v] : commStencil.pad) {
                            auto _target_k = k;
                            _target_k.l = l;
                            _target_k.p = p;
                            if (auto iter = extendedStencil.pad.findFirst(
                                        [&](auto&& _k) { return _k.l == l && _k.idx == _target_k.idx; });
                                iter == extendedStencil.pad.end()) {
                                extendedStencil.pad[_target_k] = 0;
                            }
                        }
                        std::cout << std::format("extended stencil:{}\n", extendedStencil);
                        std::vector<Real> vals;
                        std::vector<int> entries(commStencil.pad.size());
                        std::iota(entries.begin(), entries.end(), 0);
                        for (const auto& [key, val] : commStencil.pad) {
                            auto _k = key;
                            _k.l = l;
                            auto iter = extendedStencil.pad.findFirst(
                                    [&](auto&& k) { return k.l == l && k.idx == _k.idx; });
                            vals.push_back(iter->second);
                        }
                        HYPRE_SStructMatrixSetValues(A, l, i.c_arr(), 0, commStencil.pad.size(),
                                                     entries.data(), vals.data());
                        // inter-level part
                        int count = commStencil.pad.size();
                        for (const auto& [k, v] : extendedStencil.pad) {
                            if (k.l != l) {
                                int entry[] = {count++};
                                Real val[] = {v};
                                HYPRE_SStructMatrixSetValues(A, l, i.c_arr(), 0, 1, entry, val);
                                std::cout << std::format("A[{}, {}] = {}\n", i, k, v);
                            }
                        }
                        auto bias = -extendedStencil.bias;
                        HYPRE_SStructVectorSetValues(b, l, i.c_arr(), 0, &bias);
                    });
                }
            }
            int (*rfactors)[HYPRE_MAXDIM];
            rfactors = new int[target->getLevels()][HYPRE_MAXDIM];
            for (int l = 0; l < target->getLevels(); ++l)
                for (int d = 0; d < HYPRE_MAXDIM; ++d) rfactors[l][d] = 1;
            for (int l = 1; l < target->getLevels(); ++l) {
                for (int d = 0; d < dim; ++d) rfactors[l][d] = target->mesh.refinementRatio;
            }
            for (int l = target->getLevels() - 1; l > 0; --l) {
                HYPRE_SStructFACZeroCFSten(A, grid, l, rfactors[l]);
                HYPRE_SStructFACZeroFCSten(A, grid, l);
                HYPRE_SStructFACZeroAMRMatrixData(A, l - 1, rfactors[l]);
            }
            HYPRE_SStructMatrixAssemble(A);
            std::vector<int> plevels(target->getLevels());
            std::iota(plevels.begin(), plevels.end(), 0);
            HYPRE_SStructFACZeroAMRVectorData(b, plevels.data(), rfactors);
            HYPRE_SStructVectorAssemble(b);
            delete[] rfactors;
        }

        void initx() {
            for (auto l = 0; l < target->getLevels(); ++l) {
                for (std::size_t p = 0; p < target->localRanges[l].size(); ++p) {
                    rangeFor(target->localRanges[l][p], [&](auto&& i) {
                        auto val = target->evalAt(i);
                        HYPRE_SStructVectorSetValues(x, l, i.c_arr(), 0, &val);
                    });
                }
            }
            int (*rfactors)[HYPRE_MAXDIM];
            rfactors = new int[target->getLevels()][HYPRE_MAXDIM];
            for (int l = 0; l < target->getLevels(); ++l)
                for (int d = 0; d < HYPRE_MAXDIM; ++d) rfactors[l][d] = 1;
            for (int l = 1; l < target->getLevels(); ++l) {
                for (int d = 0; d < dim; ++d) rfactors[l][d] = target->mesh.refinementRatio;
            }
            std::vector<int> plevels(target->getLevels());
            std::iota(plevels.begin(), plevels.end(), 0);
            HYPRE_SStructFACZeroAMRVectorData(x, plevels.data(), rfactors);
            HYPRE_SStructVectorAssemble(x);
            delete[] rfactors;
        }

        void returnValues() {
            for (auto l = 0; l < target->getLevels(); ++l) {
                for (std::size_t p = 0; p < target->localRanges[l].size(); ++p) {
                    rangeFor(target->localRanges[l][p], [&](auto&& i) {
                        Real val;
                        HYPRE_SStructVectorGetValues(x, l, i.c_arr(), 0, &val);
                        target->operator[](i) = val;
                    });
                }
            }
        }

        EqnSolveState solve() override {
            allocHYPRE();
            generateAb();
            initx();
            HYPRE_SStructFACSetMaxLevels(solver.getSolver(), target->getLevels());
            std::vector<int> plevels(target->getLevels());
            int (*rfactors)[HYPRE_MAXDIM];
            std::iota(plevels.begin(), plevels.end(), 0);
            rfactors = new int[plevels.size()][HYPRE_MAXDIM];
            for (int i = 0; i < HYPRE_MAXDIM; ++i) rfactors[0][i] = 1;
            for (std::size_t l = 1; l < target->localRanges.size(); ++l) {
                for (int i = 0; i < HYPRE_MAXDIM; ++i)
                    rfactors[l][i] = i < dim ? target->mesh.refinementRatio : 1;
            }
            HYPRE_SStructFACSetPLevels(solver.getSolver(), plevels.size(), plevels.data());
            HYPRE_SStructFACSetPRefinements(solver.getSolver(), plevels.size(), rfactors);
            HYPRE_SStructFACSetCoarseSolverType(solver.getSolver(), 2);
            //HYPRE_SStructFACSetLogging(solver.getSolver(), 1);
            HYPRE_SStructFACSetMaxIter(solver.getSolver(), 100);
            HYPRE_SStructFACSetTol(solver.getSolver(), 1.0e-10);
            //HYPRE_SStructFACSetPLevels(         solver.getSolver(), 2, plevels);
            //HYPRE_SStructFACSetPRefinements(    solver.getSolver(), 2, rfactors);
            HYPRE_SStructFACSetRelChange(solver.getSolver(), 0);
            HYPRE_SStructFACSetCoarseSolverType(solver.getSolver(), 2);
            HYPRE_SStructFACSetLogging(solver.getSolver(), 1);

            solver.dump(A, b);
            solver.setup(A, b, x);
            solver.solve(A, b, x);
            returnValues();
            //deallocHYPRE();
            delete[] rfactors;
            return EqnSolveState(solver.getIterNum(), solver.getFinalRes());
        }

        F getter;
        std::add_pointer_t<T> target;
        using Stencil
                = DS::StencilPad<DS::ColoredIndex<typename internal::CartAMRFieldExprTrait<T>::index_type>>;
        using Eqn = Meta::RealType<decltype(std::declval<F>()(std::declval<StencilField<T>&>()))>;
        std::unique_ptr<Eqn> equation;
        using EqExpr = Meta::RealType<decltype(equation->lhs - equation->rhs)>;
        std::unique_ptr<EqExpr> uniEqn;
        Stencil commStencil;
        std::unique_ptr<StencilField<T>> stencilField;
        bool fieldsAllocated = false;
        Solver solver;
        HYPRE_SStructStencil stencil {};
        HYPRE_SStructGrid grid {};
        HYPRE_SStructGraph graph {};
        HYPRE_SStructMatrix A {};
        HYPRE_SStructVector b {}, x {};

    private:
        bool allocated = false;
        constexpr static auto dim = internal::CartAMRFieldExprTrait<T>::dim;
        using index_type = typename internal::CartAMRFieldExprTrait<T>::index_type;
    };
}// namespace OpFlow
#endif//OPFLOW_HYPREEQNSOLVEHANDLER_HPP