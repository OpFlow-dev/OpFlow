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

using namespace OpFlow;
using namespace testing;

class BlockedMDRangeMapperTest : public Test {
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

TEST_F(BlockedMDRangeMapperTest, XFaceMap_N3) {
    std::vector<DS::Range<2>> ranges;
    auto info = makeParallelInfo();
    info.nodeInfo.node_count = 3;
    auto plan = makeParallelPlan(info, ParallelIdentifier::DistributeMem);
    auto splitMap = strategy->getSplitMap(m.getRange(), plan);
    for (const auto& m : splitMap) { ranges.push_back(DS::commonRange(m, u.getLocalWritableRange())); }
    auto mapper = DS::BlockedMDRangeMapper<2>(ranges);
    // Block 2
    ASSERT_EQ(mapper(DS::MDIndex<2> {1, 0}), 0);
    // Block 3
    ASSERT_EQ(mapper(DS::MDIndex<2> {2, 0}), 4);
    ASSERT_EQ(mapper(DS::MDIndex<2> {3, 0}), 5);
}

TEST_F(BlockedMDRangeMapperTest, XFaceMap_N4) {
    std::vector<DS::Range<2>> ranges;
    auto info = makeParallelInfo();
    info.nodeInfo.node_count = 4;
    auto plan = makeParallelPlan(info, ParallelIdentifier::DistributeMem);
    auto splitMap = strategy->getSplitMap(m.getRange(), plan);
    for (const auto& m : splitMap) { ranges.push_back(DS::commonRange(m, u.getLocalWritableRange())); }
    auto mapper = DS::BlockedMDRangeMapper<2>(ranges);
    // Block 1
    ASSERT_EQ(mapper(DS::MDIndex<2> {1, 0}), 0);
    ASSERT_EQ(mapper(DS::MDIndex<2> {1, 1}), 1);
    // Block 2
    ASSERT_EQ(mapper(DS::MDIndex<2> {2, 0}), 2);
    ASSERT_EQ(mapper(DS::MDIndex<2> {3, 0}), 3);
    ASSERT_EQ(mapper(DS::MDIndex<2> {3, 1}), 5);
    // Block 3
    ASSERT_EQ(mapper(DS::MDIndex<2> {1, 2}), 6);
    // Block 4
    ASSERT_EQ(mapper(DS::MDIndex<2> {2, 2}), 8);
    ASSERT_EQ(mapper(DS::MDIndex<2> {3, 3}), 11);
}

TEST_F(BlockedMDRangeMapperTest, CenterMap_N3) {
    std::vector<DS::Range<2>> ranges;
    auto info = makeParallelInfo();
    info.nodeInfo.node_count = 3;
    auto plan = makeParallelPlan(info, ParallelIdentifier::DistributeMem);
    auto splitMap = strategy->getSplitMap(m.getRange(), plan);
    for (const auto& m : splitMap) { ranges.push_back(DS::commonRange(m, p.getLocalWritableRange())); }
    auto mapper = DS::BlockedMDRangeMapper<2>(ranges);
    // Block 1
    ASSERT_EQ(mapper(DS::MDIndex<2> {0, 0}), 0);
    ASSERT_EQ(mapper(DS::MDIndex<2> {0, 3}), 3);
    // Block 2
    ASSERT_EQ(mapper(DS::MDIndex<2> {1, 0}), 4);
    ASSERT_EQ(mapper(DS::MDIndex<2> {1, 3}), 7);
    // Block 3
    ASSERT_EQ(mapper(DS::MDIndex<2> {2, 0}), 8);
    ASSERT_EQ(mapper(DS::MDIndex<2> {3, 3}), 15);
}

TEST_F(BlockedMDRangeMapperTest, CenterMap_N4) {
    std::vector<DS::Range<2>> ranges;
    auto info = makeParallelInfo();
    info.nodeInfo.node_count = 4;
    auto plan = makeParallelPlan(info, ParallelIdentifier::DistributeMem);
    auto splitMap = strategy->getSplitMap(m.getRange(), plan);
    for (const auto& m : splitMap) { ranges.push_back(DS::commonRange(m, p.getLocalWritableRange())); }
    auto mapper = DS::BlockedMDRangeMapper<2>(ranges);
    // Block 1
    ASSERT_EQ(mapper(DS::MDIndex<2> {0, 0}), 0);
    ASSERT_EQ(mapper(DS::MDIndex<2> {1, 1}), 3);
    // Block 2
    ASSERT_EQ(mapper(DS::MDIndex<2> {2, 0}), 4);
    ASSERT_EQ(mapper(DS::MDIndex<2> {3, 1}), 7);
    // Block 3
    ASSERT_EQ(mapper(DS::MDIndex<2> {0, 2}), 8);
    ASSERT_EQ(mapper(DS::MDIndex<2> {1, 3}), 11);
    // Block 4
    ASSERT_EQ(mapper(DS::MDIndex<2> {2, 2}), 12);
    ASSERT_EQ(mapper(DS::MDIndex<2> {3, 3}), 15);
}