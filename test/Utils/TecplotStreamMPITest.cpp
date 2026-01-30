//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2025 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#include <gmock/gmock.h>
#ifdef OPFLOW_USE_MODULE
import opflow;
#else
#include <OpFlow>
#endif

class TecIOMPITest : public virtual testing::Test {
protected:
    void SetUp() override {
        using namespace OpFlow;
        const auto n = 5;
        auto mesh = MeshBuilder<Mesh>()
                            .newMesh(n, n, n)
                            .setMeshOfDim(0, 0., 1.)
                            .setMeshOfDim(1, 0., 1.)
                            .setMeshOfDim(2, 0., 1.)
                            .build();
        std::shared_ptr<AbstractSplitStrategy<Field>> strategy = std::make_shared<EvenSplitStrategy<Field>>();
        u = ExprBuilder<Field>()
                    .setName("u")
                    .setMesh(mesh)
                    .setBC(0, DimPos::start, BCType::Dirc, 1.)
                    .setBC(0, DimPos::end, BCType::Dirc, 1.)
                    .setBC(1, DimPos::start, BCType::Dirc, 1.)
                    .setBC(1, DimPos::end, BCType::Dirc, 1.)
                    .setBC(2, DimPos::start, BCType::Dirc, 1.)
                    .setBC(2, DimPos::end, BCType::Dirc, 1.)
                    .setExt(1)
                    .setLoc(std::array {LocOnMesh ::Center, LocOnMesh ::Center, LocOnMesh::Center})
                    .setSplitStrategy(strategy)
                    .build();

        u = 0;
    }

    using Mesh = OpFlow::CartesianMesh<OpFlow::Meta::int_<3>>;
    using Field = OpFlow::CartesianField<OpFlow::Real, Mesh>;
    Field u;
};

TEST_F(TecIOMPITest, SingleField) {
    using namespace OpFlow;

    if (DS::inRange(u.getLocalWritableRange(), DS::MDIndex<3>(2, 4, 1))) u[DS::MDIndex<3>(2, 4, 1)] = 2.;
    Utils::TecplotBinaryStream uf("./u.szplt");
    uf << u;
    ASSERT_TRUE(true);
}

TEST_F(TecIOMPITest, SingleField_NewAPI) {
    using namespace OpFlow;

    rangeFor(u.getLocalWritableRange(), [&](auto&& i) { u[i] = i[0] + i[1] * 10 + i[2] * 100; });
    u.updatePadding();
    Utils::TecplotSZPLTStream uf("./u_newapi.szplt");
    uf << u;
    ASSERT_TRUE(true);
}

TEST_F(TecIOMPITest, TwoFields) {
    using namespace OpFlow;
    {
        Utils::TecplotBinaryStream uf("./u1.szplt");
        uf << u;
    }
    {
        Utils::TecplotBinaryStream uf("./u2.szplt");
        uf << u;
    }
    ASSERT_TRUE(true);
}

TEST_F(TecIOMPITest, TwoFields_NewAPI) {
    using namespace OpFlow;

    if (DS::inRange(u.getLocalWritableRange(), DS::MDIndex<3> {2, 4, 2})) u[DS::MDIndex<3>(2, 4, 2)] = 2.;
    Utils::TecplotSZPLTStream uf("./u1.szplt"), uf2("./u2.szplt");
    uf << u;
    if (DS::inRange(u.getLocalWritableRange(), DS::MDIndex<3>(2, 4, 2))) u[DS::MDIndex<3>(2, 4, 2)] = 0.;
    uf2 << u;
    ASSERT_TRUE(true);
}

TEST_F(TecIOMPITest, DumpTwoFields) {
    using namespace OpFlow;
    Utils::TecplotBinaryStream uf("./uv.szplt");
    auto v = u;
    v.name = "v";
    uf.dumpMultiple(u, v);
    ASSERT_TRUE(true);
}

TEST_F(TecIOMPITest, DumpTwoFields_NewAPI) {
    using namespace OpFlow;
    Utils::TecplotSZPLTStream uf("./uv_newapi.szplt");
    auto v = u;
    v.name = "v";
    uf.dumpMultiple(u, v);
    ASSERT_TRUE(true);
}