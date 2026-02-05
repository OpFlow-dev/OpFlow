//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2026 by the OpFlow developers
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

class H5RWTest : public virtual ::testing::Test {
public:
    H5RWTest() = default;
    ~H5RWTest() override = default;

    void SetUp() override {
        constexpr auto n = 5;
        auto mesh = MeshBuilder<Mesh>().newMesh(n, n).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();
        u = ExprBuilder<Field>()
                    .setName("u")
                    .setMesh(mesh)
                    .setBC(0, DimPos::start, BCType::Dirc, 1.)
                    .setBC(0, DimPos::end, BCType::Dirc, 1.)
                    .setBC(1, DimPos::start, BCType::Dirc, 1.)
                    .setBC(1, DimPos::end, BCType::Dirc, 1.)
                    .setLoc(std::array {LocOnMesh ::Center, LocOnMesh ::Center})
                    .build();

        u = 0;
    }

    using Mesh = OpFlow::CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;

    Field u;
};

TEST_F(H5RWTest, WriteToFile) {
    u[DS::MDIndex<2> {1, 1}] = 1.;
    Utils::H5Stream stream("./u.wtf.h5");
    stream << u;
    ASSERT_TRUE(true);
}

TEST_F(H5RWTest, SeparateFile) {
    u[DS::MDIndex<2> {1, 1}] = 1.;
    Utils::H5Stream stream("./u.sf.h5");
    stream.dumpToSeparateFile();
    stream << Utils::TimeStamp(1e-4) << u << Utils::TimeStamp(1000.) << u;
    ASSERT_TRUE(std::filesystem::exists(std::format("./u.sf_{:.6f}.h5", 1e-4))
                && std::filesystem::exists(std::format("./u.sf_{:.6f}.h5", 1000.)));
}

TEST_F(H5RWTest, ReadAfterWrite) {
    // Note: A HDF5 file cannot be hold by multiple streams at the same time.
    // Therefore, we need to close the stream before reading.
    {
        u[DS::MDIndex<2>(2, 2)] = 2.;
        Utils::H5Stream stream("./u.raw.h5");
        stream << u;
    }

    auto v = u;
    v = 0.;
    Utils::H5Stream istream("./u.raw.h5", StreamIn);
    istream >> v;
    ASSERT_EQ(v.evalAt(DS::MDIndex<2>(2, 2)), u.evalAt(DS::MDIndex<2>(2, 2)));
}

TEST_F(H5RWTest, ReadAtTime) {
    {
        u[DS::MDIndex<2>(2, 2)] = 2.;
        Utils::H5Stream stream("./u.rat.h5");
        Utils::TimeStamp t0(0);
        stream << t0 << u;
        Utils::TimeStamp t1(1);
        u[DS::MDIndex<2>(3, 3)] = 3.;
        stream << t1 << u;
    }

    auto v = u;
    v = 0.;
    Utils::H5Stream istream("./u.rat.h5", StreamIn);
    istream.moveToTime(Utils::TimeStamp(1));
    istream >> v;
    ASSERT_EQ(v.evalAt(DS::MDIndex<2>(3, 3)), u.evalAt(DS::MDIndex<2>(3, 3)));
}

TEST_F(H5RWTest, ReadAfterWriteInEqualDim) {
    auto map = DS::MDRangeMapper<2> {u.accessibleRange};
    rangeFor(u.assignableRange, [&](auto&& i) { u[i] = map(i); });
    Utils::H5Stream stream("./u.ieq.h5");
    stream << u;
    stream.close();

    auto v = u;
    v = 0.;
    Utils::H5Stream istream("./u.ieq.h5", StreamIn);
    istream >> v;
    istream.close();
    ASSERT_EQ(v.evalAt(DS::MDIndex<2>(2, 2)), u.evalAt(DS::MDIndex<2>(2, 2)));
}
