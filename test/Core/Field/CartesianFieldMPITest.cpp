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
#include <tuple>

using namespace OpFlow;

class CartesianFieldMPITest
    : public testing::TestWithParam<std::tuple<std::array<BCType, 4>, std::array<LocOnMesh, 2>>> {
protected:
    void SetUp() override {
        auto info = makeParallelInfo();
        setGlobalParallelInfo(info);
        setGlobalParallelPlan(makeParallelPlan(getGlobalParallelInfo(), ParallelIdentifier::DistributeMem));
        strategy = std::make_shared<EvenSplitStrategy<Field>>();

        int n = 17;
        m = MeshBuilder<Mesh>().newMesh(n, n).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();
    }

    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<double, Mesh>;
    std::shared_ptr<AbstractSplitStrategy<Field>> strategy;
    Mesh m;
};

TEST_P(CartesianFieldMPITest, RangeCheck) {
    const auto& [bcs, locs] = GetParam();
    auto builder = ExprBuilder<Field>().setMesh(m);
    if (isLogicalBC(bcs[0])) builder.setBC(0, DimPos::start, bcs[0]);
    else
        builder.setBC(0, DimPos::start, bcs[0], 0.);
    if (isLogicalBC(bcs[1])) builder.setBC(0, DimPos::end, bcs[1]);
    else
        builder.setBC(0, DimPos::end, bcs[1], 0.);
    if (isLogicalBC(bcs[2])) builder.setBC(1, DimPos::start, bcs[2]);
    else
        builder.setBC(1, DimPos::start, bcs[2], 0.);
    if (isLogicalBC(bcs[3])) builder.setBC(1, DimPos::end, bcs[3]);
    else
        builder.setBC(1, DimPos::end, bcs[3], 0.);

    auto u = builder.setLoc(locs).setPadding(1).setExt(1).setSplitStrategy(strategy).build();

    auto mr = strategy->splitRange(m.getRange(), getGlobalParallelPlan());
    // mpi only affect localRange
    for (int i = 0; i < 2; ++i) {
        ASSERT_EQ(u.localRange.start[i], mr.start[i]);
        int end;
        if (locs[i] == LocOnMesh::Center || bcs[i] == BCType::Periodic) end = mr.end[i];
        else if (locs[i] == LocOnMesh::Corner && mr.end[i] + 1 != u.accessibleRange.end[i])
            end = mr.end[i];
        else
            end = mr.end[i] + 1;
        ASSERT_EQ(u.localRange.end[i], end);
    }
}

// should behave the same as no-ext version
TEST_P(CartesianFieldMPITest, WithExtRangeCheck) {
    const auto& [bcs, locs] = GetParam();
    auto builder = ExprBuilder<Field>().setMesh(m);
    if (isLogicalBC(bcs[0])) builder.setBC(0, DimPos::start, bcs[0]);
    else
        builder.setBC(0, DimPos::start, bcs[0], 0.);
    if (isLogicalBC(bcs[1])) builder.setBC(0, DimPos::end, bcs[1]);
    else
        builder.setBC(0, DimPos::end, bcs[1], 0.);
    if (isLogicalBC(bcs[2])) builder.setBC(1, DimPos::start, bcs[2]);
    else
        builder.setBC(1, DimPos::start, bcs[2], 0.);
    if (isLogicalBC(bcs[3])) builder.setBC(1, DimPos::end, bcs[3]);
    else
        builder.setBC(1, DimPos::end, bcs[3], 0.);

    auto u = builder.setLoc(locs).setPadding(1).setExt(3).setSplitStrategy(strategy).build();

    auto mr = strategy->splitRange(m.getRange(), getGlobalParallelPlan());
    // mpi only affect localRange
    for (int i = 0; i < 2; ++i) {
        ASSERT_EQ(u.localRange.start[i], mr.start[i]);
        int end;
        if (locs[i] == LocOnMesh::Center || bcs[i] == BCType::Periodic) end = mr.end[i];
        else if (locs[i] == LocOnMesh::Corner && mr.end[i] + 1 != u.accessibleRange.end[i])
            end = mr.end[i];
        else
            end = mr.end[i] + 1;
        ASSERT_EQ(u.localRange.end[i], end);
    }
}

TEST_F(CartesianFieldMPITest, PeriodicValueCheck) {
    auto u = ExprBuilder<Field>()
                     .setMesh(m)
                     .setBC(0, OpFlow::DimPos::start, OpFlow::BCType::Periodic)
                     .setBC(0, OpFlow::DimPos::end, OpFlow::BCType::Periodic)
                     .setBC(1, OpFlow::DimPos::start, OpFlow::BCType::Periodic)
                     .setBC(1, OpFlow::DimPos::end, OpFlow::BCType::Periodic)
                     .setPadding(1)
                     .setExt(1)
                     .setSplitStrategy(strategy)
                     .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                     .build();
    auto u_local = ExprBuilder<Field>()
                           .setMesh(m)
                           .setBC(0, OpFlow::DimPos::start, OpFlow::BCType::Periodic)
                           .setBC(0, OpFlow::DimPos::end, OpFlow::BCType::Periodic)
                           .setBC(1, OpFlow::DimPos::start, OpFlow::BCType::Periodic)
                           .setBC(1, OpFlow::DimPos::end, OpFlow::BCType::Periodic)
                           .setExt(1)
                           .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                           .build();

    auto mapper = DS::MDRangeMapper<2>(u.assignableRange);
    rangeFor_s(u.getLocalWritableRange(), [&](auto&& i) { u[i] = mapper(i); });
    rangeFor_s(u_local.getLocalWritableRange(), [&](auto&& i) { u_local[i] = mapper(i); });
    u.updatePadding();
    u_local.updatePadding();
    rangeFor_s(u.getLocalReadableRange(), [&](auto&& i) {
        if (u[i] != u_local[i]) OP_INFO("Not equal at {} {} != {}", i, u[i], u_local[i]);
        ASSERT_EQ(u[i], u_local[i]);
    });
}

TEST_F(CartesianFieldMPITest, Serializable_PeriodicValueCheck) {
    class Int : public virtual SerializableObj {
    public:
        int i = 0;
        Int() = default;
        explicit Int(int i) : i(i) {}
        [[nodiscard]] std::vector<std::byte> serialize() const override {
            return {reinterpret_cast<const std::byte*>(&i), reinterpret_cast<const std::byte*>(&i + 1)};
        }
        void deserialize(const std::byte* data, std::size_t size) override {
            std::memcpy(&i, data, sizeof(i));
        }
    };
    auto s = std::make_shared<EvenSplitStrategy<CartesianField<Int, Mesh>>>();
    auto u = ExprBuilder<CartesianField<Int, Mesh>>()
                     .setMesh(m)
                     .setPadding(1)
                     .setName("uInt")
                     .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                     .setSplitStrategy(s)
                     .build();

    auto mapper = DS::MDRangeMapper<2>(u.assignableRange);
    rangeFor_s(u.getLocalWritableRange(), [&](auto&& i) { u[i] = Int {mapper(i)}; });
    u.updatePadding();
    rangeFor_s(u.getLocalReadableRange(), [&](auto&& i) {
        if (u[i].i != mapper(i)) OP_INFO("Not equal at {} {} != {}", i, u[i].i, mapper(i));
        ASSERT_EQ(u[i].i, mapper(i));
    });
}

INSTANTIATE_TEST_SUITE_P(
        Param2D, CartesianFieldMPITest,
        testing::Values(std::make_tuple(std::array {BCType::Dirc, BCType::Dirc, BCType::Dirc, BCType::Dirc},
                                        std::array {LocOnMesh::Center, LocOnMesh::Center}),
                        std::make_tuple(std::array {BCType::Neum, BCType::Neum, BCType::Neum, BCType::Neum},
                                        std::array {LocOnMesh ::Center, LocOnMesh::Center}),
                        std::make_tuple(std::array {BCType::Periodic, BCType::Periodic, BCType::Periodic,
                                                    BCType::Periodic},
                                        std::array {LocOnMesh ::Center, LocOnMesh::Center}),
                        std::make_tuple(std::array {BCType::Dirc, BCType::Dirc, BCType::Dirc, BCType::Dirc},
                                        std::array {LocOnMesh ::Corner, LocOnMesh::Corner}),
                        std::make_tuple(std::array {BCType::Neum, BCType::Neum, BCType::Neum, BCType::Neum},
                                        std::array {LocOnMesh ::Corner, LocOnMesh::Corner}),
                        std::make_tuple(std::array {BCType::Periodic, BCType::Periodic, BCType::Periodic,
                                                    BCType::Periodic},
                                        std::array {LocOnMesh ::Corner, LocOnMesh::Corner})));
