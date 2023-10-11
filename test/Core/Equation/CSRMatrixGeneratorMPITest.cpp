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

#include <OpFlow>
#include <gmock/gmock.h>

using namespace OpFlow;
using namespace testing;

class CSRMatrixGeneratorMPITest : public Test {
protected:
    ParallelPlan ori_plan;
    void SetUp() override {
        m = MeshBuilder<Mesh>().newMesh(5, 5).setMeshOfDim(0, 0., 4.).setMeshOfDim(1, 0., 4.).build();
        strategy = std::make_shared<EvenSplitStrategy<Field>>();
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

    DS::CSRMatrix gather_mat(const DS::CSRMatrix& mat_local) const {
        if (getWorkerId() == 0) {
            DS::CSRMatrix mat_global;
            mat_global.append(mat_local);
            for (int i = 1; i < getWorkerCount(); ++i) {
                DS::CSRMatrix mat_tmp;
                int row_size, col_size, val_size, rhs_size;
                MPI_Recv(&row_size, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&col_size, 1, MPI_INT, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&val_size, 1, MPI_INT, i, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&rhs_size, 1, MPI_INT, i, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                mat_tmp.row.resize(row_size);
                mat_tmp.col.resize(col_size);
                mat_tmp.val.resize(val_size);
                mat_tmp.rhs.resize(rhs_size);
                MPI_Recv(mat_tmp.row.raw(), row_size, MPI_LONG, i, 4, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(mat_tmp.col.raw(), col_size, MPI_LONG, i, 5, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(mat_tmp.val.raw(), val_size, MPI_DOUBLE, i, 6, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(mat_tmp.rhs.data(), rhs_size, MPI_DOUBLE, i, 7, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                mat_global.append(mat_tmp);
            }
            return mat_global;
        } else {
            int row_size = mat_local.row.size();
            int col_size = mat_local.col.size();
            int val_size = mat_local.val.size();
            int rhs_size = mat_local.rhs.size();
            MPI_Send(&row_size, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            MPI_Send(&col_size, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
            MPI_Send(&val_size, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
            MPI_Send(&rhs_size, 1, MPI_INT, 0, 3, MPI_COMM_WORLD);
            MPI_Send(mat_local.row.raw(), row_size, MPI_LONG, 0, 4, MPI_COMM_WORLD);
            MPI_Send(mat_local.col.raw(), col_size, MPI_LONG, 0, 5, MPI_COMM_WORLD);
            MPI_Send(mat_local.val.raw(), val_size, MPI_DOUBLE, 0, 6, MPI_COMM_WORLD);
            MPI_Send(mat_local.rhs.data(), rhs_size, MPI_DOUBLE, 0, 7, MPI_COMM_WORLD);
        }
        return mat_local;
    }

    auto simple_poisson() {
        return [&](auto&& e) { return 1.0 == d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e); };
    }

    auto simple_helmholtz() {
        return [&](auto&& e) {
            return 1.0 == e + d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e);
        };
    }

    static void assert_mat_eq(const DS::CSRMatrix& mat1, const DS::CSRMatrix& mat2) {
        ASSERT_EQ(mat1.row.size(), mat2.row.size());
        ASSERT_EQ(mat1.col.size(), mat2.col.size());
        ASSERT_EQ(mat1.val.size(), mat2.val.size());
        ASSERT_EQ(mat1.rhs.size(), mat2.rhs.size());
        for (int i = 0; i < mat1.row.size(); ++i) { ASSERT_EQ(mat1.row[i], mat2.row[i]); }
        for (int i = 0; i < mat1.col.size(); ++i) { ASSERT_EQ(mat1.col[i], mat2.col[i]); }
        for (int i = 0; i < mat1.val.size(); ++i) { ASSERT_EQ(mat1.val[i], mat2.val[i]); }
        for (int i = 0; i < mat1.rhs.size(); ++i) { ASSERT_EQ(mat1.rhs[i], mat2.rhs[i]); }
    }

    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;
    Mesh m;
    Field p, r, b, u;
    std::shared_ptr<AbstractSplitStrategy<Field>> strategy;
};

TEST_F(CSRMatrixGeneratorMPITest, SimplePoisson) {
    p = ExprBuilder<Field>()
                .setMesh(m)
                .setName("p")
                .setBC(0, DimPos::start, BCType::Dirc, 0.)
                .setBC(0, DimPos::end, BCType::Dirc, 0.)
                .setBC(1, DimPos::start, BCType::Dirc, 0.)
                .setBC(1, DimPos::end, BCType::Dirc, 0.)
                .setExt(1)
                .setPadding(1)
                .setSplitStrategy(strategy)
                .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                .build();

    auto eqn = makeEqnHolder(std::forward_as_tuple(simple_poisson()), std::forward_as_tuple(p));
    auto st = makeStencilHolder(eqn);
    auto mapper = DS::BlockedMDRangeMapper<2> {p.getLocalWritableRange()};
    auto mat = CSRMatrixGenerator::generate<0>(st, mapper, false);
    auto mat_global = gather_mat(mat);

    if (getWorkerId() == 0) {
        auto p_local = ExprBuilder<Field>()
                               .setMesh(m)
                               .setName("p")
                               .setBC(0, DimPos::start, BCType::Dirc, 0.)
                               .setBC(0, DimPos::end, BCType::Dirc, 0.)
                               .setBC(1, DimPos::start, BCType::Dirc, 0.)
                               .setBC(1, DimPos::end, BCType::Dirc, 0.)
                               .setExt(1)
                               .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                               .build();
        auto eqn_local
                = makeEqnHolder(std::forward_as_tuple(simple_poisson()), std::forward_as_tuple(p_local));
        auto st_local = makeStencilHolder(eqn_local);
        auto mat_local = CSRMatrixGenerator::generate<0>(st_local, mapper, false);

        assert_mat_eq(mat_global, mat_local);
    } else {
        ASSERT_TRUE(true);
    }
}

TEST_F(CSRMatrixGeneratorMPITest, SimplePoisson_Neum) {
    p = ExprBuilder<Field>()
                .setMesh(m)
                .setName("p")
                .setBC(0, DimPos::start, BCType::Neum, 0.)
                .setBC(0, DimPos::end, BCType::Neum, 0.)
                .setBC(1, DimPos::start, BCType::Neum, 0.)
                .setBC(1, DimPos::end, BCType::Neum, 0.)
                .setExt(1)
                .setPadding(1)
                .setSplitStrategy(strategy)
                .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                .build();

    auto eqn = makeEqnHolder(std::forward_as_tuple(simple_poisson()), std::forward_as_tuple(p));
    auto st = makeStencilHolder(eqn);
    auto mapper = DS::BlockedMDRangeMapper<2> {p.getLocalWritableRange()};
    auto mat = CSRMatrixGenerator::generate<0>(st, mapper, true);

    auto mat_global = gather_mat(mat);

    if (getWorkerId() == 0) {
        auto p_local = ExprBuilder<Field>()
                               .setMesh(m)
                               .setName("p")
                               .setBC(0, DimPos::start, BCType::Neum, 0.)
                               .setBC(0, DimPos::end, BCType::Neum, 0.)
                               .setBC(1, DimPos::start, BCType::Neum, 0.)
                               .setBC(1, DimPos::end, BCType::Neum, 0.)
                               .setExt(1)
                               .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                               .build();
        auto eqn_local
                = makeEqnHolder(std::forward_as_tuple(simple_poisson()), std::forward_as_tuple(p_local));
        auto st_local = makeStencilHolder(eqn_local);
        auto mat_local = CSRMatrixGenerator::generate<0>(st_local, mapper, true);
        assert_mat_eq(mat_global, mat_local);
    } else {
        ASSERT_TRUE(true);
    }
}

TEST_F(CSRMatrixGeneratorMPITest, SimplePoisson_Periodic) {
    p = ExprBuilder<Field>()
                .setMesh(m)
                .setName("p")
                .setBC(0, DimPos::start, BCType::Periodic)
                .setBC(0, DimPos::end, BCType::Periodic)
                .setBC(1, DimPos::start, BCType::Periodic)
                .setBC(1, DimPos::end, BCType::Periodic)
                .setExt(1)
                .setPadding(1)
                .setSplitStrategy(strategy)
                .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                .build();

    auto eqn = makeEqnHolder(std::forward_as_tuple(simple_poisson()), std::forward_as_tuple(p));
    auto st = makeStencilHolder(eqn);
    auto mapper = DS::BlockedMDRangeMapper<2> {p.getLocalWritableRange()};
    auto mat = CSRMatrixGenerator::generate<0>(st, mapper, true);

    auto mat_global = gather_mat(mat);

    if (getWorkerId() == 0) {
        auto p_local = ExprBuilder<Field>()
                               .setMesh(m)
                               .setName("p")
                               .setBC(0, DimPos::start, BCType::Periodic)
                               .setBC(0, DimPos::end, BCType::Periodic)
                               .setBC(1, DimPos::start, BCType::Periodic)
                               .setBC(1, DimPos::end, BCType::Periodic)
                               .setExt(1)
                               .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                               .build();
        auto eqn_local
                = makeEqnHolder(std::forward_as_tuple(simple_poisson()), std::forward_as_tuple(p_local));
        auto st_local = makeStencilHolder(eqn_local);
        auto mat_local = CSRMatrixGenerator::generate<0>(st_local, mapper, true);
        assert_mat_eq(mat_global, mat_local);
    } else {
        ASSERT_TRUE(true);
    }
}

TEST_F(CSRMatrixGeneratorMPITest, SimplePoisson_2Eqn) {
    p = ExprBuilder<Field>()
                .setMesh(m)
                .setName("p")
                .setBC(0, DimPos::start, BCType::Dirc, 0.)
                .setBC(0, DimPos::end, BCType::Dirc, 0.)
                .setBC(1, DimPos::start, BCType::Dirc, 0.)
                .setBC(1, DimPos::end, BCType::Dirc, 0.)
                .setExt(1)
                .setPadding(1)
                .setSplitStrategy(strategy)
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
    DS::ColoredBlockedMDRangeMapper<2> mapper {p.getLocalWritableRange(), p.getLocalWritableRange()};
    std::vector<bool> pin {false, false};

    auto mat = CSRMatrixGenerator::generate(st, mapper, pin);

    for (int i = mat.row.size() / 2, j = 0; i < mat.row.size(); ++i, ++j) {
        ASSERT_EQ(mat.row[i] - mat.row[mat.row.size() / 2], mat.row[j]);
    }
    for (int i = mat.col.size() / 2, j = 0; i < mat.col.size(); ++i, ++j) {
        if (getWorkerCount() == 3) {// non-even split case
            int offset;
            if (4 <= mat.col[i] && mat.col[i] < 16) offset = 4;
            else
                offset = 8;
            ASSERT_EQ(mat.col[i] - offset, mat.col[j]);
        } else {
            // other case (N = 1, 2, 4) are even split, the offset is the same
            ASSERT_EQ(mat.col[i] - p.getLocalWritableRange().count(), mat.col[j]);
        }
    }
    for (int i = mat.val.size() / 2, j = 0; i < mat.val.size(); ++i, ++j) {
        ASSERT_DOUBLE_EQ(mat.val[i], mat.val[j]);
    }
    for (int i = mat.rhs.size() / 2, j = 0; i < mat.rhs.size(); ++i, ++j) {
        ASSERT_DOUBLE_EQ(mat.rhs[i], mat.rhs[j]);
    }
}

TEST_F(CSRMatrixGeneratorMPITest, SimplePoisson_Neum_2Eqn) {
    p = ExprBuilder<Field>()
                .setMesh(m)
                .setName("p")
                .setBC(0, DimPos::start, BCType::Neum, 0.)
                .setBC(0, DimPos::end, BCType::Neum, 0.)
                .setBC(1, DimPos::start, BCType::Neum, 0.)
                .setBC(1, DimPos::end, BCType::Neum, 0.)
                .setExt(1)
                .setPadding(1)
                .setSplitStrategy(strategy)
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
    DS::ColoredBlockedMDRangeMapper<2> mapper {p.getLocalWritableRange(), p.getLocalWritableRange()};
    std::vector<bool> pin {true, true};

    auto mat = CSRMatrixGenerator::generate(st, mapper, pin);

    for (int i = mat.row.size() / 2, j = 0; i < mat.row.size(); ++i, ++j) {
        ASSERT_EQ(mat.row[i] - mat.row[mat.row.size() / 2], mat.row[j]);
    }
    for (int i = mat.col.size() / 2, j = 0; i < mat.col.size(); ++i, ++j) {
        if (getWorkerCount() == 3) {// non-even split case
            int offset;
            if (4 <= mat.col[i] && mat.col[i] < 16) offset = 4;
            else
                offset = 8;
            ASSERT_EQ(mat.col[i] - offset, mat.col[j]);
        } else {
            // other case (N = 1, 2, 4) are even split, the offset is the same
            ASSERT_EQ(mat.col[i] - p.getLocalWritableRange().count(), mat.col[j]);
        }
    }
    for (int i = mat.val.size() / 2, j = 0; i < mat.val.size(); ++i, ++j) {
        ASSERT_DOUBLE_EQ(mat.val[i], mat.val[j]);
    }
    for (int i = mat.rhs.size() / 2, j = 0; i < mat.rhs.size(); ++i, ++j) {
        ASSERT_DOUBLE_EQ(mat.rhs[i], mat.rhs[j]);
    }
}

TEST_F(CSRMatrixGeneratorMPITest, XFaceHelmholtz) {
    u = ExprBuilder<Field>()
                .setMesh(m)
                .setName("u")
                .setBC(0, DimPos::start, BCType::Dirc, 0.)
                .setBC(0, DimPos::end, BCType::Dirc, 0.)
                .setBC(1, DimPos::start, BCType::Dirc, 0.)
                .setBC(1, DimPos::end, BCType::Dirc, 0.)
                .setExt(1)
                .setPadding(1)
                .setSplitStrategy(strategy)
                .setLoc({LocOnMesh::Corner, LocOnMesh::Center})
                .build();

    auto eqn = makeEqnHolder(std::forward_as_tuple(simple_helmholtz()), std::forward_as_tuple(u));
    auto st = makeStencilHolder(eqn);
    auto mapper = DS::BlockedMDRangeMapper<2>(u.getLocalWritableRange());
    auto mat = CSRMatrixGenerator::generate<0>(st, mapper, false);

    auto mat_global = gather_mat(mat);

    if (getWorkerId() == 0) {
        auto u_local = ExprBuilder<Field>()
                               .setMesh(m)
                               .setName("u")
                               .setBC(0, DimPos::start, BCType::Dirc, 0.)
                               .setBC(0, DimPos::end, BCType::Dirc, 0.)
                               .setBC(1, DimPos::start, BCType::Dirc, 0.)
                               .setBC(1, DimPos::end, BCType::Dirc, 0.)
                               .setExt(1)
                               .setLoc({LocOnMesh::Corner, LocOnMesh::Center})
                               .build();
        auto eqn_local
                = makeEqnHolder(std::forward_as_tuple(simple_helmholtz()), std::forward_as_tuple(u_local));
        auto st_local = makeStencilHolder(eqn_local);
        auto mat_local = CSRMatrixGenerator::generate<0>(st_local, mapper, false);
        assert_mat_eq(mat_global, mat_local);
    } else {
        ASSERT_TRUE(true);
    }
}

TEST_F(CSRMatrixGeneratorMPITest, XFaceHelmholtz_Periodic) {
    u = ExprBuilder<Field>()
                .setMesh(m)
                .setName("u")
                .setBC(0, DimPos::start, BCType::Periodic, 0.)
                .setBC(0, DimPos::end, BCType::Periodic, 0.)
                .setBC(1, DimPos::start, BCType::Periodic, 0.)
                .setBC(1, DimPos::end, BCType::Periodic, 0.)
                .setExt(1)
                .setPadding(1)
                .setSplitStrategy(strategy)
                .setLoc({LocOnMesh::Corner, LocOnMesh::Center})
                .build();

    auto eqn = makeEqnHolder(std::forward_as_tuple(simple_helmholtz()), std::forward_as_tuple(u));
    auto st = makeStencilHolder(eqn);
    auto mapper = DS::BlockedMDRangeMapper<2>(u.getLocalWritableRange());
    auto mat = CSRMatrixGenerator::generate<0>(st, mapper, false);

    auto mat_global = gather_mat(mat);

    if (getWorkerId() == 0) {
        auto u_local = ExprBuilder<Field>()
                               .setMesh(m)
                               .setName("u")
                               .setBC(0, DimPos::start, BCType::Periodic, 0.)
                               .setBC(0, DimPos::end, BCType::Periodic, 0.)
                               .setBC(1, DimPos::start, BCType::Periodic, 0.)
                               .setBC(1, DimPos::end, BCType::Periodic, 0.)
                               .setExt(1)
                               .setLoc({LocOnMesh::Corner, LocOnMesh::Center})
                               .build();
        auto eqn_local
                = makeEqnHolder(std::forward_as_tuple(simple_helmholtz()), std::forward_as_tuple(u_local));
        auto st_local = makeStencilHolder(eqn_local);
        auto mat_local = CSRMatrixGenerator::generate<0>(st_local, mapper, false);
        assert_mat_eq(mat_global, mat_local);
    } else {
        ASSERT_TRUE(true);
    }
}