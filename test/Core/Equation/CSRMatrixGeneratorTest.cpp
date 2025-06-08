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

//#define OPFLOW_ENABLE_STACK_TRACE 1
#include <gmock/gmock.h>
#include <print>
import opflow;

using namespace OpFlow;
using namespace testing;

class CSRMatrixGeneratorTest : public Test {
protected:
    ParallelPlan ori_plan;
    void SetUp() override {
        m = MeshBuilder<Mesh>().newMesh(5, 5).setMeshOfDim(0, 0., 4.).setMeshOfDim(1, 0., 4.).build();
    }

    void reset_case(double xc, double yc) {
        r.initBy([&](auto&& x) {
            auto dist = Math::norm2(x[0] - xc, x[1] - yc);
            auto hevi = Math::smoothHeviside(r.getMesh().dx(0, 0) * 8, dist - 0.2);
            return 1. * hevi + (1. - hevi) * 1000;
        });
        b = 1.0;
        p = 0.;
    }

    auto poisson_eqn() {
        return [&](auto&& e) {
            return b
                   == dx<D1FirstOrderCentered>(dx<D1FirstOrderCentered>(e) / d1IntpCenterToCorner<0>((r)))
                              + dy<D1FirstOrderCentered>(dy<D1FirstOrderCentered>(e)
                                                         / d1IntpCenterToCorner<1>(r));
        };
    }

    auto simple_poisson() {
        return [&](auto&& e) { return 1.0 == d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e); };
    }

    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;
    Mesh m;
    Field p, r, b;
};

TEST_F(CSRMatrixGeneratorTest, SimplePoisson) {
    p = ExprBuilder<Field>()
                .setMesh(m)
                .setName("p")
                .setBC(0, DimPos::start, BCType::Dirc, 0.)
                .setBC(0, DimPos::end, BCType::Dirc, 0.)
                .setBC(1, DimPos::start, BCType::Dirc, 0.)
                .setBC(1, DimPos::end, BCType::Dirc, 0.)
                .setExt(0, DimPos::start, 1)
                .setExt(0, DimPos::end, 1)
                .setExt(1, DimPos::start, 1)
                .setExt(1, DimPos::end, 1)
                .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                .build();

    auto eqn = makeEqnHolder(std::forward_as_tuple(simple_poisson()), std::forward_as_tuple(p));
    auto st = makeStencilHolder(eqn);
    auto mat = CSRMatrixGenerator::generate<0>(st, DS::ColoredMDRangeMapper<2> {p.assignableRange}, false);

    std::vector<int> ptr {0, 3, 7, 11, 14, 18, 23, 28, 32, 36, 41, 46, 50, 53, 57, 61, 64},
            col {0,  1,  4, 0,  1,  2,  5, 1,  2,  3,  6,  2,  3,  7,  0,  4,  5,  8,  1,  4, 5, 6,
                 9,  2,  5, 6,  7,  10, 3, 6,  7,  11, 4,  8,  9,  12, 5,  8,  9,  10, 13, 6, 9, 10,
                 11, 14, 7, 10, 11, 15, 8, 12, 13, 9,  12, 13, 14, 10, 13, 14, 15, 11, 14, 15};
    std::vector<double> val {6,  -1, -1, -1, 5,  -1, -1, -1, 5,  -1, -1, -1, 6,  -1, -1, 5,
                             -1, -1, -1, -1, 4,  -1, -1, -1, -1, 4,  -1, -1, -1, -1, 5,  -1,
                             -1, 5,  -1, -1, -1, -1, 4,  -1, -1, -1, -1, 4,  -1, -1, -1, -1,
                             5,  -1, -1, 6,  -1, -1, -1, 5,  -1, -1, -1, 5,  -1, -1, -1, 6},
            rhs {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

    for (int i = 0; i < ptr.size(); ++i) { ASSERT_EQ(ptr[i], mat.row[i]); }
    for (int i = 0; i < col.size(); ++i) { ASSERT_EQ(col[i], mat.col[i]); }
    for (int i = 0; i < val.size(); ++i) { ASSERT_DOUBLE_EQ(val[i], mat.val[i]); }
    for (int i = 0; i < rhs.size(); ++i) { ASSERT_DOUBLE_EQ(rhs[i], mat.rhs[i]); }
}

TEST_F(CSRMatrixGeneratorTest, SimplePoisson_Neum) {
    p = ExprBuilder<Field>()
                .setMesh(m)
                .setName("p")
                .setBC(0, DimPos::start, BCType::Neum, 0.)
                .setBC(0, DimPos::end, BCType::Neum, 0.)
                .setBC(1, DimPos::start, BCType::Neum, 0.)
                .setBC(1, DimPos::end, BCType::Neum, 0.)
                .setExt(0, DimPos::start, 1)
                .setExt(0, DimPos::end, 1)
                .setExt(1, DimPos::start, 1)
                .setExt(1, DimPos::end, 1)
                .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                .build();

    auto eqn = makeEqnHolder(std::forward_as_tuple(simple_poisson()), std::forward_as_tuple(p));
    auto st = makeStencilHolder(eqn);
    auto mat = CSRMatrixGenerator::generate<0>(st, DS::ColoredMDRangeMapper<2> {p.assignableRange}, true);

    std::vector<int> ptr {0, 3, 7, 11, 14, 18, 23, 28, 32, 36, 41, 46, 50, 53, 57, 61, 62},
            col {0, 1,  4,  0,  1, 2,  5,  1,  2, 3,  6,  2, 3,  7,  0,  4,  5,  8,  1,  4,  5,
                 6, 9,  2,  5,  6, 7,  10, 3,  6, 7,  11, 4, 8,  9,  12, 5,  8,  9,  10, 13, 6,
                 9, 10, 11, 14, 7, 10, 11, 15, 8, 12, 13, 9, 12, 13, 14, 10, 13, 14, 15, 15};
    std::vector<double> val {2,  -1, -1, -1, 3,  -1, -1, -1, 3,  -1, -1, -1, 2,  -1, -1, 3,
                             -1, -1, -1, -1, 4,  -1, -1, -1, -1, 4,  -1, -1, -1, -1, 3,  -1,
                             -1, 3,  -1, -1, -1, -1, 4,  -1, -1, -1, -1, 4,  -1, -1, -1, -1,
                             3,  -1, -1, 2,  -1, -1, -1, 3,  -1, -1, -1, 3,  -1, 1},
            rhs {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0};

    for (int i = 0; i < ptr.size(); ++i) { ASSERT_EQ(ptr[i], mat.row[i]); }
    for (int i = 0; i < col.size(); ++i) { ASSERT_EQ(col[i], mat.col[i]); }
    for (int i = 0; i < val.size(); ++i) { ASSERT_DOUBLE_EQ(val[i], mat.val[i]); }
    for (int i = 0; i < rhs.size(); ++i) { ASSERT_DOUBLE_EQ(rhs[i], mat.rhs[i]); }
}

TEST_F(CSRMatrixGeneratorTest, SimplePoisson_Periodic) {
    p = ExprBuilder<Field>()
                .setMesh(m)
                .setName("p")
                .setBC(0, DimPos::start, BCType::Periodic)
                .setBC(0, DimPos::end, BCType::Periodic)
                .setBC(1, DimPos::start, BCType::Periodic)
                .setBC(1, DimPos::end, BCType::Periodic)
                .setExt(1)
                .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                .build();

    auto eqn = makeEqnHolder(std::forward_as_tuple(simple_poisson()), std::forward_as_tuple(p));
    auto st = makeStencilHolder(eqn);
    auto mat = CSRMatrixGenerator::generate<0>(st, DS::ColoredMDRangeMapper<2> {p.assignableRange}, true);

    std::vector<int> ptr {0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 76},
            col {0,  1,  3,  4, 12, 0,  1,  2,  5, 13, 1,  2,  3,  6, 14, 0,  2,  3,  7,
                 15, 0,  4,  5, 7,  8,  1,  4,  5, 6,  9,  2,  5,  6, 7,  10, 3,  4,  6,
                 7,  11, 4,  8, 9,  11, 12, 5,  8, 9,  10, 13, 6,  9, 10, 11, 14, 7,  8,
                 10, 11, 15, 0, 8,  12, 13, 15, 1, 9,  12, 13, 14, 2, 10, 13, 14, 15, 15};
    std::vector<double> val {4,  -1, -1, -1, -1, -1, 4,  -1, -1, -1, -1, 4,  -1, -1, -1, -1, -1, 4,  -1,
                             -1, -1, 4,  -1, -1, -1, -1, -1, 4,  -1, -1, -1, -1, 4,  -1, -1, -1, -1, -1,
                             4,  -1, -1, 4,  -1, -1, -1, -1, -1, 4,  -1, -1, -1, -1, 4,  -1, -1, -1, -1,
                             -1, 4,  -1, -1, -1, 4,  -1, -1, -1, -1, -1, 4,  -1, -1, -1, -1, 4,  -1, 1},
            rhs {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0};

    std::print("{}", mat.toString());

    for (int i = 0; i < ptr.size(); ++i) { ASSERT_EQ(ptr[i], mat.row[i]); }
    for (int i = 0; i < col.size(); ++i) { ASSERT_EQ(col[i], mat.col[i]); }
    for (int i = 0; i < val.size(); ++i) { ASSERT_DOUBLE_EQ(val[i], mat.val[i]); }
    for (int i = 0; i < rhs.size(); ++i) { ASSERT_DOUBLE_EQ(rhs[i], mat.rhs[i]); }
    ASSERT_TRUE(true);
}

TEST_F(CSRMatrixGeneratorTest, SimplePoisson_2Eqn) {
    p = ExprBuilder<Field>()
                .setMesh(m)
                .setName("p")
                .setBC(0, DimPos::start, BCType::Dirc, 0.)
                .setBC(0, DimPos::end, BCType::Dirc, 0.)
                .setBC(1, DimPos::start, BCType::Dirc, 0.)
                .setBC(1, DimPos::end, BCType::Dirc, 0.)
                .setExt(0, DimPos::start, 1)
                .setExt(0, DimPos::end, 1)
                .setExt(1, DimPos::start, 1)
                .setExt(1, DimPos::end, 1)
                .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                .build();

    auto eqn = makeEqnHolder(
            std::forward_as_tuple(
                    [&](auto&& e, auto&&) {
                        return 1.0 == d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e);
                    },
                    [&](auto&&, auto&& e) {
                        return 1.0 == d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e);
                    }),
            std::forward_as_tuple(p, p));
    auto st = makeStencilHolder(eqn);
    DS::ColoredMDRangeMapper<2> mapper {p.assignableRange, p.assignableRange};
    std::vector<bool> pin {false, false};

    auto mat = CSRMatrixGenerator::generate(st, mapper, pin);

    for (int i = mat.row.size() / 2, j = 0; i < mat.row.size(); ++i, ++j) {
        ASSERT_EQ(mat.row[i] - mat.row[mat.row.size() / 2], mat.row[j]);
    }
    for (int i = mat.col.size() / 2, j = 0; i < mat.col.size(); ++i, ++j) {
        ASSERT_EQ(mat.col[i] - 16, mat.col[j]);
    }
    for (int i = mat.val.size() / 2, j = 0; i < mat.val.size(); ++i, ++j) {
        ASSERT_DOUBLE_EQ(mat.val[i], mat.val[j]);
    }
    for (int i = mat.rhs.size() / 2, j = 0; i < mat.rhs.size(); ++i, ++j) {
        ASSERT_DOUBLE_EQ(mat.rhs[i], mat.rhs[j]);
    }
}

TEST_F(CSRMatrixGeneratorTest, SimplePoisson_Neum_2Eqn) {
    p = ExprBuilder<Field>()
                .setMesh(m)
                .setName("p")
                .setBC(0, DimPos::start, BCType::Neum, 0.)
                .setBC(0, DimPos::end, BCType::Neum, 0.)
                .setBC(1, DimPos::start, BCType::Neum, 0.)
                .setBC(1, DimPos::end, BCType::Neum, 0.)
                .setExt(1)
                .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                .build();

    auto eqn = makeEqnHolder(
            std::forward_as_tuple(
                    [&](auto&& e, auto&&) {
                        return 1.0 == d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e);
                    },
                    [&](auto&&, auto&& e) {
                        return 1.0 == d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e);
                    }),
            std::forward_as_tuple(p, p));
    auto st = makeStencilHolder(eqn);
    DS::ColoredMDRangeMapper<2> mapper {p.assignableRange, p.assignableRange};
    std::vector<bool> pin {true, true};

    auto mat = CSRMatrixGenerator::generate(st, mapper, pin);

    for (int i = mat.row.size() / 2, j = 0; i < mat.row.size(); ++i, ++j) {
        ASSERT_EQ(mat.row[i] - mat.row[mat.row.size() / 2], mat.row[j]);
    }
    for (int i = mat.col.size() / 2, j = 0; i < mat.col.size(); ++i, ++j) {
        ASSERT_EQ(mat.col[i] - 16, mat.col[j]);
    }
    for (int i = mat.val.size() / 2, j = 0; i < mat.val.size(); ++i, ++j) {
        ASSERT_DOUBLE_EQ(mat.val[i], mat.val[j]);
    }
    for (int i = mat.rhs.size() / 2, j = 0; i < mat.rhs.size(); ++i, ++j) {
        ASSERT_DOUBLE_EQ(mat.rhs[i], mat.rhs[j]);
    }
}