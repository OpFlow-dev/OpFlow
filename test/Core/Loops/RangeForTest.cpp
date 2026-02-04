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

#include <gmock/gmock.h>
#include <iostream>
import opflow;

using namespace OpFlow;
using namespace testing;

class RangedIndexTest : public Test {
protected:
    void SetUp() override {
        index.range = DS::Range<2>({1, 1}, {10, 10}, {3, 2});
        index.set(index.range.start);
    }

    [[nodiscard]] auto fromLinear(int linear) const {
        DS::MDIndex<decltype(index)::dim> ret;
        ret.set(index.range.start);
        assert(linear >= 0);
        for (auto i = 0; i < index.dim; ++i) {
            ret[i] += linear % ((index.range.end[i] - 1 - index.range.start[i]) / index.range.stride[i] + 1)
                      * index.range.stride[i];
            linear /= ((index.range.end[i] - 1 - index.range.start[i]) / index.range.stride[i] + 1);
        }
        return ret;
    }

    template <typename T>
    requires Meta::BracketIndexable<T>&& requires(T t) {
        t.dim;
    }
    [[nodiscard]] auto serialize(const T& idx) const {
        auto ret = std::format("({}", idx[0]);
        for (auto i = 1; i < idx.dim; ++i) { ret += std::format(", {}", idx[i]); }
        ret += ")";
        return ret;
    }

    DS::RangedIndex<2> index;
    bool debug = false;
};

TEST_F(RangedIndexTest, GetTotalCount) {
    auto total = 1;
    for (auto i = 0; i < index.dim; ++i) {
        total *= (index.range.end[i] - index.range.start[i] - 1) / index.range.stride[i] + 1;
    }
    ASSERT_EQ(index.count(), total);
}

TEST_F(RangedIndexTest, MoveToNext) {
    auto total = index.count();
    for (auto linear = 0; linear < total; ++linear, ++index) {
        auto target = fromLinear(linear);
        if (debug)
            std::cout << std::format("Lin. = {:>3d}, Target = {}, Current = {}", linear, serialize(target),
                                     serialize(index))
                      << std::endl;
        for (auto i = 0; i < index.dim; ++i) { ASSERT_EQ(target[i], index[i]); }
    }
}

TEST_F(RangedIndexTest, GetLast) {
    auto total = index.count();
    auto target = fromLinear(total - 1);
    auto last = index.last();
    if (debug)
        std::cout << std::format("Lin. = {:>3d}, Target = {}, Current = {}", total - 1, serialize(target),
                                 serialize(last))
                  << std::endl;
    for (auto i = 0; i < index.dim; ++i) { ASSERT_EQ(target[i], last[i]); }
}

TEST_F(RangedIndexTest, MoveToPre) {
    auto total = index.count();
    index = index.last();
    for (auto linear = total - 1; linear >= 0; --linear, --index) {
        auto target = fromLinear(linear);
        if (debug)
            std::cout << std::format("Lin. = {:>3d}, Target = {}, Current = {}", linear, serialize(target),
                                     serialize(index))
                      << std::endl;
        for (auto i = 0; i < index.dim; ++i) { ASSERT_EQ(target[i], index[i]); }
    }
}

TEST_F(RangedIndexTest, OneAfterLastReturnEnd) {
    auto last = index.last();
    ++last;
    for (auto i = 0; i < index.dim; ++i) { ASSERT_EQ(last[i], index.range.end[i]); }
}

TEST_F(RangedIndexTest, OneBeforeFirstReturnFirst) {
    auto first = index.first();
    --first;
    for (auto i = 0; i < index.dim; ++i) { ASSERT_EQ(first[i], index.range.start[i]); }
}

class RangeForTest : public Test {
protected:
    void SetUp() override {
        auto info = makeParallelInfo();
        setGlobalParallelInfo(info);
        setGlobalParallelPlan(makeParallelPlan(info, ParallelIdentifier::SharedMem));
    }
};

TEST_F(RangeForTest, 1D) {
    DS::Range<1> r {{100}};
    std::vector<int> a(100, 0);
    rangeFor(r, [&](auto&& k) { a[k[0]]++; });
    for (auto& i : a) { ASSERT_EQ(i, 1); }
}

TEST_F(RangeForTest, 2D) {
    DS::Range<2> r {{100, 100}};
    int a[100][100];
    for (int i = 0; i < 100; ++i)
        for (int j = 0; j < 100; ++j) a[i][j] = 0;
    rangeFor(r, [&](auto&& k) { a[k[0]][k[1]]++; });
    for (int i = 0; i < 100; ++i)
        for (int j = 0; j < 100; ++j) { ASSERT_EQ(a[i][j], 1); }
}

TEST_F(RangeForTest, 2D2) {
    DS::Range<2> r {{1, 1}, {100, 100}};
    int a[100][100];
    for (int i = 0; i < 100; ++i)
        for (int j = 0; j < 100; ++j) a[i][j] = 0;
    rangeFor(r, [&](auto&& k) { a[k[0]][k[1]]++; });
    for (int i = 0; i < 100; ++i) {
        for (int j = 0; j < 100; ++j) {
            if (1 <= i && 1 <= j) {
                ASSERT_EQ(a[i][j], 1);
            } else {
                ASSERT_EQ(a[i][j], 0);
            }
        }
    }
}

TEST_F(RangeForTest, 3D) {
    DS::Range<3> r {{100, 100, 100}};
    int a[100][100][100];
    for (int i = 0; i < 100; ++i)
        for (int j = 0; j < 100; ++j)
            for (int k = 0; k < 100; ++k) a[i][j][k] = 0;
    rangeFor(r, [&](auto&& k) { a[k[0]][k[1]][k[2]]++; });
    for (int i = 0; i < 100; ++i)
        for (int j = 0; j < 100; ++j)
            for (int k = 0; k < 100; ++k) { ASSERT_EQ(a[i][j][k], 1); }
}