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

#include <OpFlow>
#include <gmock/gmock.h>

using namespace OpFlow;
using namespace testing;

class ColoredBlockedMDRangeMapperTest : public Test {
protected:
    void SetUp() override {
        m = MeshBuilder<Mesh>().newMesh(5, 5).setMeshOfDim(0, 0., 4.).setMeshOfDim(1, 0., 4.).build();
        strategy = std::make_shared<EvenSplitStrategy<Field>>();
        u = ExprBuilder<Field>()
                    .setMesh(m)
                    .setName("u")
                    .setBC(0, DimPos::start, BCType::Dirc, 0.)
                    .setBC(0, DimPos::end, BCType::Dirc, 0.)
                    .setBC(1, DimPos::start, BCType::Dirc, 0.)
                    .setBC(1, DimPos::end, BCType::Dirc, 0.)
                    .setExt(1)
                    .setPadding(1)
                    .setLoc({LocOnMesh::Corner, LocOnMesh::Center})
                    .build();
        p = ExprBuilder<Field>()
                    .setMesh(m)
                    .setName("p")
                    .setBC(0, DimPos::start, BCType::Dirc, 0.)
                    .setBC(0, DimPos::end, BCType::Dirc, 0.)
                    .setBC(1, DimPos::start, BCType::Dirc, 0.)
                    .setBC(1, DimPos::end, BCType::Dirc, 0.)
                    .setExt(1)
                    .setPadding(1)
                    .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                    .build();
    }

    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;
    Field u, p;
    std::shared_ptr<AbstractSplitStrategy<Field>> strategy;
    Mesh m;
};

TEST_F(ColoredBlockedMDRangeMapperTest, XFaceMap) {
    std::vector<DS::Range<2>> ranges;
    auto info = makeParallelInfo();
    info.nodeInfo.node_count = 3;
    auto plan = makeParallelPlan(info, ParallelIdentifier::DistributeMem);
    auto splitMap = strategy->getSplitMap(m.getRange(), plan);
    for (const auto& m : splitMap) { ranges.push_back(DS::commonRange(m, u.getLocalWritableRange())); }
    auto mapper = DS::ColoredBlockedMDRangeMapper<2>(ranges);
    // Block 2
    ASSERT_EQ(mapper(DS::ColoredIndex<DS::MDIndex<2>>{DS::MDIndex<2>{1, 0}, 0}), 0);
    // Block 3
    ASSERT_EQ(mapper(DS::ColoredIndex<DS::MDIndex<2>>{DS::MDIndex<2>{2, 0}, 0}), 4);
    ASSERT_EQ(mapper(DS::ColoredIndex<DS::MDIndex<2>>{DS::MDIndex<2>{3, 0}, 0}), 5);
}