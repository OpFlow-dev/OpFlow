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

class H5RWMPITest : public virtual ::testing::Test {
public:
    H5RWMPITest() = default;
    ~H5RWMPITest() override = default;

    void SetUp() override {
        constexpr auto n = 5;
        auto mesh = MeshBuilder<Mesh>().newMesh(n, n).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();
        std::shared_ptr<AbstractSplitStrategy<Field>> strategy = std::make_shared<EvenSplitStrategy<Field>>();
        u = ExprBuilder<Field>()
                    .setName("u")
                    .setMesh(mesh)
                    .setBC(0, DimPos::start, BCType::Dirc, 1.)
                    .setBC(0, DimPos::end, BCType::Dirc, 1.)
                    .setBC(1, DimPos::start, BCType::Dirc, 1.)
                    .setBC(1, DimPos::end, BCType::Dirc, 1.)
                    .setPadding(1)
                    .setSplitStrategy(strategy)
                    .setLoc(std::array {LocOnMesh ::Center, LocOnMesh ::Center})
                    .build();

        u = 0;
    }

    using Mesh = OpFlow::CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;

    Field u;
};

TEST_F(H5RWMPITest, WriteToFile) {
    u = 1.;
    std::string filename = fmt::format("./u.wrf.mpi{}.h5", getWorkerCount());
    Utils::H5Stream stream(filename);
    stream << u;
    ASSERT_TRUE(true);
}

TEST_F(H5RWMPITest, SeparateFile) {
    u = 1.;
    Utils::H5Stream stream(fmt::format("./u.sf.mpi{}.h5", getWorkerCount()));
    stream.dumpToSeparateFile();
    stream << Utils::TimeStamp(1e-4) << u << Utils::TimeStamp(1000.) << u;
    ASSERT_TRUE(std::filesystem::exists(fmt::format("./u.sf.mpi{}_{:.6f}.h5", getWorkerCount(), 1e-4))
                && std::filesystem::exists(fmt::format("./u.sf.mpi{}_{:.6f}.h5", getWorkerCount(), 1000.)));
}

TEST_F(H5RWMPITest, ReadAfterWrite) {
    // Note: A HDF5 file cannot be hold by multiple streams at the same time.
    // Therefore, we need to close the stream before reading.
    if (DS::inRange(u.getLocalWritableRange(), DS::MDIndex<2> {2, 2})) u[DS::MDIndex<2>(2, 2)] = 2.;
    u.updatePadding();
    std::string filename = fmt::format("./u.raw.mpi{}.h5", getWorkerCount());

    {
        Utils::H5Stream stream(filename);
        stream << u;
    }

    auto v = u;
    v = 0.;
    Utils::H5Stream istream(filename, StreamIn);
    istream >> v;
    rangeFor_s(u.getLocalReadableRange(), [&](auto i) {
        if (u[i] != v[i]) OP_ERROR("NOT EQUAL AT {}", i);
        ASSERT_EQ(u[i], v[i]);
    });
}

TEST_F(H5RWMPITest, ReadAtTime) {
    std::string filename = fmt::format("./u.rat.mpi{}.h5", getWorkerCount());

    {
        Utils::H5Stream stream(filename);
        if (DS::inRange(u.getLocalWritableRange(), DS::MDIndex<2>(2, 2))) { u[DS::MDIndex<2>(2, 2)] = 2.; }
        Utils::TimeStamp t0(0);
        stream << t0 << u;
        if (DS::inRange(u.getLocalWritableRange(), DS::MDIndex<2>(3, 3))) { u[DS::MDIndex<2>(3, 3)] = 3.; }
        Utils::TimeStamp t1(1);
        stream << t1 << u;
    }
    u.updatePadding();

    auto v = u;
    v = 0.;
    Utils::H5Stream istream(filename, StreamIn);
    istream.moveToTime(Utils::TimeStamp(1));
    istream >> v;

    rangeFor_s(u.getLocalReadableRange(), [&](auto&& i) { ASSERT_EQ(v.evalAt(i), u.evalAt(i)); });
}

TEST_F(H5RWMPITest, ReadAfterWriteInEqualDim) {
    auto map = DS::MDRangeMapper<2> {u.accessibleRange};
    rangeFor(u.getLocalWritableRange(), [&](auto&& i) { u[i] = map(i); });
    u.updatePadding();
    std::string filename = fmt::format("./u.ieq.mpi{}.h5", getWorkerCount());

    Utils::H5Stream stream(filename);
    stream << u;
    stream.close();

    auto v = u;
    v = 0.;
    Utils::H5Stream istream(filename, StreamIn);
    istream >> v;
    istream.close();
    rangeFor_s(u.getLocalReadableRange(), [&](auto&& i) { ASSERT_EQ(v.evalAt(i), u.evalAt(i)); });
}

TEST_F(H5RWMPITest, GeneralSplitWite) {
    if (getWorkerCount() != 4) GTEST_SKIP();
    auto s = std::make_shared<ManualSplitStrategy<Field>>();
    s->splitMap.emplace_back(std::array {0, 0}, std::array {1, 4});
    s->splitMap.emplace_back(std::array {3, 0}, std::array {4, 4});
    s->splitMap.emplace_back(std::array {1, 2}, std::array {3, 4});
    s->splitMap.emplace_back(std::array {1, 0}, std::array {3, 2});
    u.initBy([](auto&& x) { return getWorkerId(); });

    Utils::H5Stream stream("./u.general.split.h5");
    stream << Utils::TimeStamp(0) << u;
    u.resplitWithStrategy(s.get());
    OP_INFO("local range = {}", u.localRange.toString());
    stream << Utils::TimeStamp(1.) << u;
    u.initBy([](auto&& x) { return getWorkerId(); });
    stream << Utils::TimeStamp(2) << u;
    auto es = std::make_shared<EvenSplitStrategy<Field>>();
    u.resplitWithStrategy(es.get());
    stream << Utils::TimeStamp(3) << u;
    ASSERT_TRUE(true);
}