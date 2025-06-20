/*
    Copyright (c) 2005-2022 Intel Corporation

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "common/test.h"
#include "common/utils.h"
#include "common/utils_assert.h"
#include "common/utils_concurrency_limit.h"

#include "oneapi/tbb/blocked_range3d.h"
#include "oneapi/tbb/global_control.h"
#include "oneapi/tbb/parallel_for.h"

#include <vector>

//! \file conformance_blocked_range3d.cpp
//! \brief Test for [algorithms.blocked_range3d] specification

template <typename Tag>
class AbstractValueType {
    AbstractValueType() {}
    int value;

public:
    template <typename OtherTag>
    friend AbstractValueType<OtherTag> MakeAbstractValueType(int i);

    template <typename OtherTag>
    friend int GetValueOf(const AbstractValueType<OtherTag>& v);
};

template <typename Tag>
AbstractValueType<Tag> MakeAbstractValueType(int i) {
    AbstractValueType<Tag> x;
    x.value = i;
    return x;
}

template <typename Tag>
int GetValueOf(const AbstractValueType<Tag>& v) {
    return v.value;
}

template <typename Tag>
bool operator<(const AbstractValueType<Tag>& u, const AbstractValueType<Tag>& v) {
    return GetValueOf(u) < GetValueOf(v);
}

template <typename Tag>
std::size_t operator-(const AbstractValueType<Tag>& u, const AbstractValueType<Tag>& v) {
    return GetValueOf(u) - GetValueOf(v);
}

template <typename Tag>
AbstractValueType<Tag> operator+(const AbstractValueType<Tag>& u, std::size_t offset) {
    return MakeAbstractValueType<Tag>(GetValueOf(u) + int(offset));
}

struct PageTag {};
struct RowTag {};
struct ColTag {};

static void SerialTest() {
    typedef AbstractValueType<PageTag> page_type;
    typedef AbstractValueType<RowTag> row_type;
    typedef AbstractValueType<ColTag> col_type;
    typedef oneapi::tbb::blocked_range3d<page_type, row_type, col_type> range_type;
    for (int page_x = -4; page_x < 4; ++page_x) {
        for (int page_y = page_x; page_y < 4; ++page_y) {
            page_type page_i = MakeAbstractValueType<PageTag>(page_x);
            page_type page_j = MakeAbstractValueType<PageTag>(page_y);
            for (int page_grain = 1; page_grain < 4; ++page_grain) {
                for (int row_x = -4; row_x < 4; ++row_x) {
                    for (int row_y = row_x; row_y < 4; ++row_y) {
                        row_type row_i = MakeAbstractValueType<RowTag>(row_x);
                        row_type row_j = MakeAbstractValueType<RowTag>(row_y);
                        for (int row_grain = 1; row_grain < 4; ++row_grain) {
                            for (int col_x = -4; col_x < 4; ++col_x) {
                                for (int col_y = col_x; col_y < 4; ++col_y) {
                                    col_type col_i = MakeAbstractValueType<ColTag>(col_x);
                                    col_type col_j = MakeAbstractValueType<ColTag>(col_y);
                                    for (int col_grain = 1; col_grain < 4; ++col_grain) {
                                        range_type r(page_i, page_j, page_grain, row_i, row_j, row_grain,
                                                     col_i, col_j, col_grain);
                                        utils::AssertSameType(r.is_divisible(), true);

                                        utils::AssertSameType(r.empty(), true);

                                        utils::AssertSameType(
                                                static_cast<range_type::page_range_type::const_iterator*>(
                                                        nullptr),
                                                static_cast<page_type*>(nullptr));
                                        utils::AssertSameType(
                                                static_cast<range_type::row_range_type::const_iterator*>(
                                                        nullptr),
                                                static_cast<row_type*>(nullptr));
                                        utils::AssertSameType(
                                                static_cast<range_type::col_range_type::const_iterator*>(
                                                        nullptr),
                                                static_cast<col_type*>(nullptr));

                                        utils::AssertSameType(
                                                r.pages(),
                                                oneapi::tbb::blocked_range<page_type>(page_i, page_j, 1));
                                        utils::AssertSameType(r.rows(), oneapi::tbb::blocked_range<row_type>(
                                                                                row_i, row_j, 1));
                                        utils::AssertSameType(r.cols(), oneapi::tbb::blocked_range<col_type>(
                                                                                col_i, col_j, 1));

                                        REQUIRE(r.empty()
                                                == (page_x == page_y || row_x == row_y || col_x == col_y));

                                        REQUIRE(r.is_divisible()
                                                == (page_y - page_x > page_grain || row_y - row_x > row_grain
                                                    || col_y - col_x > col_grain));

                                        if (r.is_divisible()) {
                                            range_type r2(r, oneapi::tbb::split());
                                            if ((GetValueOf(r2.pages().begin())
                                                 == GetValueOf(r.pages().begin()))
                                                && (GetValueOf(r2.rows().begin())
                                                    == GetValueOf(r.rows().begin()))) {
                                                REQUIRE(GetValueOf(r2.pages().end())
                                                        == GetValueOf(r.pages().end()));
                                                REQUIRE(GetValueOf(r2.rows().end())
                                                        == GetValueOf(r.rows().end()));
                                                REQUIRE(GetValueOf(r2.cols().begin())
                                                        == GetValueOf(r.cols().end()));
                                            } else {
                                                if ((GetValueOf(r2.pages().begin())
                                                     == GetValueOf(r.pages().begin()))
                                                    && (GetValueOf(r2.cols().begin())
                                                        == GetValueOf(r.cols().begin()))) {
                                                    REQUIRE(GetValueOf(r2.pages().end())
                                                            == GetValueOf(r.pages().end()));
                                                    REQUIRE(GetValueOf(r2.cols().end())
                                                            == GetValueOf(r.cols().end()));
                                                    REQUIRE(GetValueOf(r2.rows().begin())
                                                            == GetValueOf(r.rows().end()));
                                                } else {
                                                    REQUIRE(GetValueOf(r2.rows().end())
                                                            == GetValueOf(r.rows().end()));
                                                    REQUIRE(GetValueOf(r2.cols().end())
                                                            == GetValueOf(r.cols().end()));
                                                    REQUIRE(GetValueOf(r2.pages().begin())
                                                            == GetValueOf(r.pages().end()));
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

const int N = 1 << 5;

unsigned char Array[N][N][N];

struct Striker {
    // Note: we use <int> here instead of <long> in order to test for problems similar to Quad 407676
    void operator()(const oneapi::tbb::blocked_range3d<int>& r) const {
        for (oneapi::tbb::blocked_range<int>::const_iterator i = r.pages().begin(); i != r.pages().end(); ++i)
            for (oneapi::tbb::blocked_range<int>::const_iterator j = r.rows().begin(); j != r.rows().end();
                 ++j)
                for (oneapi::tbb::blocked_range<int>::const_iterator k = r.cols().begin();
                     k != r.cols().end(); ++k)
                    ++Array[i][j][k];
    }
};

void ParallelTest() {
    for (int i = 0; i < N; i = i < 3 ? i + 1 : i * 3) {
        for (int j = 0; j < N; j = j < 3 ? j + 1 : j * 3) {
            for (int k = 0; k < N; k = k < 3 ? k + 1 : k * 3) {
                const oneapi::tbb::blocked_range3d<int> r(0, i, 5, 0, j, 3, 0, k, 1);
                oneapi::tbb::parallel_for(r, Striker());
                for (int l = 0; l < N; ++l) {
                    for (int m = 0; m < N; ++m) {
                        for (int n = 0; n < N; ++n) {
                            if (Array[l][m][n] != (l < i && m < j && n < k)) REQUIRE(false);
                            Array[l][m][n] = 0;
                        }
                    }
                }
            }
        }
    }
}

//! Testing blocked_range3d interface
//! \brief \ref interface \ref requirement
TEST_CASE("Serial test") { SerialTest(); }

//! Testing blocked_range3d interface with parallel_for
//! \brief \ref requirement
TEST_CASE("Parallel test") {
    for (auto concurrency_level : utils::concurrency_range()) {
        oneapi::tbb::global_control control(oneapi::tbb::global_control::max_allowed_parallelism,
                                            concurrency_level);
        ParallelTest();
    }
}

//! Testing blocked_range3d with proportional splitting
//! \brief \ref interface \ref requirement
TEST_CASE("blocked_range3d proportional splitting") {
    oneapi::tbb::blocked_range3d<int> original(0, 100, 0, 100, 0, 100);
    oneapi::tbb::blocked_range3d<int> first(original);
    oneapi::tbb::proportional_split ps(3, 1);
    oneapi::tbb::blocked_range3d<int> second(first, ps);

    int expected_first_end = static_cast<int>(original.rows().begin()
                                              + ps.left() * (original.rows().end() - original.rows().begin())
                                                        / (ps.left() + ps.right()));
    if (first.rows().size() == second.rows().size()) {
        if (first.cols().size() == second.cols().size()) {
            // Splitting was made by pages
            utils::check_range_bounds_after_splitting(original.pages(), first.pages(), second.pages(),
                                                      expected_first_end);
        } else {
            // Splitting was made by cols
            utils::check_range_bounds_after_splitting(original.cols(), first.cols(), second.cols(),
                                                      expected_first_end);
        }
    } else {
        // Splitting was made by rows
        utils::check_range_bounds_after_splitting(original.rows(), first.rows(), second.rows(),
                                                  expected_first_end);
    }
}

#if __TBB_CPP17_DEDUCTION_GUIDES_PRESENT
//! Testing blocked_range3d deduction guides
//! \brief \ref interface
TEST_CASE("Deduction guides") {
    std::vector<const unsigned long*> v;
    std::vector<double> v2;
    std::vector<std::vector<int>> v3;

    // check blocked_range2d(PageValue, PageValue, size_t, RowValue, RowValue, size_t, ColValue, ColValue, size_t)
    oneapi::tbb::blocked_range3d r1(v.begin(), v.end(), 2, v2.begin(), v2.end(), 2, v3.begin(), v3.end(), 6);
    static_assert(std::is_same<decltype(r1),
                               oneapi::tbb::blocked_range3d<decltype(v)::iterator, decltype(v2)::iterator,
                                                            decltype(v3)::iterator>>::value);

    // check blocked_range2d(blocked_range3d &)
    oneapi::tbb::blocked_range3d r2(r1);
    static_assert(std::is_same<decltype(r2), decltype(r1)>::value);

    // check blocked_range2d(blocked_range3d &&)
    oneapi::tbb::blocked_range3d r3(std::move(r1));
    static_assert(std::is_same<decltype(r3), decltype(r1)>::value);
}
#endif
