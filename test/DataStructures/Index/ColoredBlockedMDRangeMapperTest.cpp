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
import opflow;

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

// Single target. Should fallback to BlockedMDRangeMapper.
TEST_F(ColoredBlockedMDRangeMapperTest, XFaceMap_N3_Fallback) {
    std::vector<DS::Range<2>> ranges;
    auto info = makeParallelInfo();
    info.nodeInfo.node_count = 3;
    auto plan = makeParallelPlan(info, ParallelIdentifier::DistributeMem);
    auto splitMap = strategy->getSplitMap(m.getRange(), plan);
    for (const auto& m : splitMap) { ranges.push_back(DS::commonRange(m, u.getLocalWritableRange())); }
    auto mapper = DS::ColoredBlockedMDRangeMapper<2>(ranges);
    auto fallback_mapper = DS::BlockedMDRangeMapper<2>(ranges);
    rangeFor_s(u.getLocalWritableRange(), [&](auto&& i) {
        ASSERT_EQ(mapper(DS::ColoredIndex<DS::MDIndex<2>> {i, 0}), fallback_mapper(i));
    });
}

TEST_F(ColoredBlockedMDRangeMapperTest, XFaceMap_N4_Fallback) {
    std::vector<DS::Range<2>> ranges;
    auto info = makeParallelInfo();
    info.nodeInfo.node_count = 4;
    auto plan = makeParallelPlan(info, ParallelIdentifier::DistributeMem);
    auto splitMap = strategy->getSplitMap(m.getRange(), plan);
    for (const auto& m : splitMap) { ranges.push_back(DS::commonRange(m, u.getLocalWritableRange())); }
    auto mapper = DS::ColoredBlockedMDRangeMapper<2>(ranges);
    auto fallback_mapper = DS::BlockedMDRangeMapper<2>(ranges);
    rangeFor_s(u.getLocalWritableRange(), [&](auto&& i) {
        ASSERT_EQ(mapper(DS::ColoredIndex<DS::MDIndex<2>> {i, 0}), fallback_mapper(i));
    });
}

TEST_F(ColoredBlockedMDRangeMapperTest, CenterMap_N3_Fallback) {
    std::vector<DS::Range<2>> ranges;
    auto info = makeParallelInfo();
    info.nodeInfo.node_count = 3;
    auto plan = makeParallelPlan(info, ParallelIdentifier::DistributeMem);
    auto splitMap = strategy->getSplitMap(m.getRange(), plan);
    for (const auto& m : splitMap) { ranges.push_back(DS::commonRange(m, p.getLocalWritableRange())); }
    auto mapper = DS::ColoredBlockedMDRangeMapper<2>(ranges);
    auto fallback_mapper = DS::BlockedMDRangeMapper<2>(ranges);
    rangeFor_s(p.getLocalWritableRange(), [&](auto&& i) {
        ASSERT_EQ(mapper(DS::ColoredIndex<DS::MDIndex<2>> {i, 0}), fallback_mapper(i));
    });
}

TEST_F(ColoredBlockedMDRangeMapperTest, CenterMap_N4_Fallback) {
    std::vector<DS::Range<2>> ranges;
    auto info = makeParallelInfo();
    info.nodeInfo.node_count = 4;
    auto plan = makeParallelPlan(info, ParallelIdentifier::DistributeMem);
    auto splitMap = strategy->getSplitMap(m.getRange(), plan);
    for (const auto& m : splitMap) { ranges.push_back(DS::commonRange(m, p.getLocalWritableRange())); }
    auto mapper = DS::ColoredBlockedMDRangeMapper<2>(ranges);
    auto fallback_mapper = DS::BlockedMDRangeMapper<2>(ranges);
    rangeFor_s(p.getLocalWritableRange(), [&](auto&& i) {
        ASSERT_EQ(mapper(DS::ColoredIndex<DS::MDIndex<2>> {i, 0}), fallback_mapper(i));
    });
}

TEST_F(ColoredBlockedMDRangeMapperTest, TwoTarget_SingleProc) {
    std::vector<DS::Range<2>> ranges1, ranges2;
    auto info = makeParallelInfo();
    info.nodeInfo.node_count = 1;
    auto plan = makeParallelPlan(info, ParallelIdentifier::DistributeMem);
    auto splitMap = strategy->getSplitMap(m.getRange(), plan);
    for (const auto& m : splitMap) { ranges1.push_back(DS::commonRange(m, p.getLocalWritableRange())); }
    ranges2 = ranges1;
    auto mapper = DS::ColoredBlockedMDRangeMapper<2>(ranges1, ranges2);
    auto fallback_mapper = DS::BlockedMDRangeMapper<2>(ranges1);
    // for the first target, should rank as normal
    rangeFor_s(p.getLocalWritableRange(), [&](auto&& i) {
        ASSERT_EQ(mapper(DS::ColoredIndex<DS::MDIndex<2>> {i, 0}), fallback_mapper(i));
    });
    // for the second target, should rank with an offset of the size of the first target
    rangeFor_s(p.getLocalWritableRange(), [&](auto&& i) {
        ASSERT_EQ(mapper(DS::ColoredIndex<DS::MDIndex<2>> {i, 1}),
                  fallback_mapper(i) + p.getLocalWritableRange().count());
    });
}

TEST_F(ColoredBlockedMDRangeMapperTest, TwoTarget_TwoProc) {
    std::vector<DS::Range<2>> ranges1, ranges2;
    auto info = makeParallelInfo();
    info.nodeInfo.node_count = 2;
    auto plan = makeParallelPlan(info, ParallelIdentifier::DistributeMem);
    auto splitMap = strategy->getSplitMap(m.getRange(), plan);
    for (const auto& m : splitMap) { ranges1.push_back(DS::commonRange(m, p.getLocalWritableRange())); }
    ranges2 = ranges1;
    auto mapper = DS::ColoredBlockedMDRangeMapper<2>(ranges1, ranges2);
    auto fallback_mapper = DS::BlockedMDRangeMapper<2>(ranges1);
    // the rank should be interleaved, i.e., [ target0.block0, target1.block0, target0.block1, target1.block1 ]
    // target0.block0
    rangeFor_s(ranges1[0], [&](auto&& i) {
        ASSERT_EQ(mapper(DS::ColoredIndex<DS::MDIndex<2>> {i, 0}), fallback_mapper(i));
    });
    // target1.block0
    rangeFor_s(ranges2[0], [&](auto&& i) {
        ASSERT_EQ(mapper(DS::ColoredIndex<DS::MDIndex<2>> {i, 1}), fallback_mapper(i) + ranges1[0].count());
    });
    // target0.block1
    rangeFor_s(ranges1[1], [&](auto&& i) {
        // only add range2[0].count() because fallback mapper already counts for all ranges in ranges1
        ASSERT_EQ(mapper(DS::ColoredIndex<DS::MDIndex<2>> {i, 0}), fallback_mapper(i) + ranges2[0].count());
    });
    // target1.block1
    rangeFor_s(ranges2[1], [&](auto&& i) {
        ASSERT_EQ(mapper(DS::ColoredIndex<DS::MDIndex<2>> {i, 1}),
                  fallback_mapper(i) + ranges1[0].count() + ranges1[1].count());
    });
}

TEST_F(ColoredBlockedMDRangeMapperTest, TwoTarget_ThreeProc) {
    std::vector<DS::Range<2>> ranges1, ranges2;
    auto info = makeParallelInfo();
    info.nodeInfo.node_count = 3;
    auto plan = makeParallelPlan(info, ParallelIdentifier::DistributeMem);
    auto splitMap = strategy->getSplitMap(m.getRange(), plan);
    for (const auto& m : splitMap) { ranges1.push_back(DS::commonRange(m, p.getLocalWritableRange())); }
    ranges2 = ranges1;
    auto mapper = DS::ColoredBlockedMDRangeMapper<2>(ranges1, ranges2);
    auto fallback_mapper = DS::BlockedMDRangeMapper<2>(ranges1);
    // the rank should be interleaved, i.e., [ target0.block0, target1.block0, target0.block1,
    // target1.block1, target0.block2, target1.block2 ]
    // target0.block0
    rangeFor_s(ranges1[0], [&](auto&& i) {
        ASSERT_EQ(mapper(DS::ColoredIndex<DS::MDIndex<2>> {i, 0}), fallback_mapper(i));
    });
    // target1.block0
    rangeFor_s(ranges2[0], [&](auto&& i) {
        ASSERT_EQ(mapper(DS::ColoredIndex<DS::MDIndex<2>> {i, 1}), fallback_mapper(i) + ranges1[0].count());
    });
    // target0.block1
    rangeFor_s(ranges1[1], [&](auto&& i) {
        // only add range2[0].count() because fallback mapper already counts for all ranges in ranges1
        ASSERT_EQ(mapper(DS::ColoredIndex<DS::MDIndex<2>> {i, 0}), fallback_mapper(i) + ranges2[0].count());
    });
    // target1.block1
    rangeFor_s(ranges2[1], [&](auto&& i) {
        ASSERT_EQ(mapper(DS::ColoredIndex<DS::MDIndex<2>> {i, 1}),
                  fallback_mapper(i) + ranges1[0].count() + ranges1[1].count());
    });
    // target0.block2
    rangeFor_s(ranges1[2], [&](auto&& i) {
        ASSERT_EQ(mapper(DS::ColoredIndex<DS::MDIndex<2>> {i, 0}),
                  fallback_mapper(i) + ranges2[0].count() + ranges2[1].count());
    });
    // target1.block2
    rangeFor_s(ranges2[2], [&](auto&& i) {
        ASSERT_EQ(mapper(DS::ColoredIndex<DS::MDIndex<2>> {i, 1}),
                  fallback_mapper(i) + ranges1[0].count() + ranges1[1].count() + ranges1[2].count());
    });
}
