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

#ifndef OPFLOW_AMRGEN_HPP
#define OPFLOW_AMRGEN_HPP

#include "Core/Loops/RangeFor.hpp"
#include "Core/Meta.hpp"
#include "DataStructures/Arrays/OffsetVector.hpp"
#include "DataStructures/Geometry/BasicElements.hpp"
#include "DataStructures/Geometry/KdTree.hpp"
#include "DataStructures/Range/Ranges.hpp"
#include <algorithm>
#include <deque>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkXMLPolyDataWriter.h>

namespace OpFlow {
    inline auto int_pow(int a, int n) {
        auto ret = 1;
        for (auto i = 0; i < n; ++i) ret *= a;
        return ret;
    }

    auto getFillRate(const auto &marker, const auto &box) {
        constexpr static auto dim = Meta::RealType<decltype(box)>::dim;
        auto total = 1;
        for (auto i = 0; i < dim; ++i) { total *= (box.hi[i] - box.lo[i] + 1); }
        auto count = marker.countInBox(box);
        return (double) count / total;
    }

    auto domainPartition(const auto &marker, double fr_threshold, int slim_threshold = 3) {
        constexpr auto dim = Meta::RealType<decltype(marker)>::dim;
        std::deque<DS::Box<int, dim>> queue;
        // find the global AABB and push it into queue
        queue.push_back(marker.boundingBox());
        // the result vector
        std::vector<DS::Box<int, dim>> ret;
        // start the main loop
        while (!queue.empty()) {
            auto box = queue.front();
            queue.pop_front();
            auto fillRate = getFillRate(marker, box);
            // if the box is empty, continue
            if (fillRate == 0) continue;
            if (fillRate >= fr_threshold) {
                // take the box
                ret.push_back(box);
                goto next_cycle;
            } else {
                // calculate the signature
                // init arrays
                std::array<DS::OffsetVector<int>, dim> sigs;
                for (auto i = 0; i < dim; ++i) {
                    sigs[i].resize(box.hi[i] - box.lo[i] + 1);
                    sigs[i].setConstant(0);
                    sigs[i].setOffset(box.lo[i]);
                }
                // traverse all the points & accumulate sigs
                marker.traverseInBox(box, [&](auto &&p) {
                    for (auto i = 0; i < dim; ++i) { sigs[i][p[i]]++; }
                });
                // find all continues non-zero intervals in sigs
                std::array<std::vector<int>, dim> split_points;
                for (auto i = 0; i < dim; ++i) {
                    const auto begin = sigs[i].begin();
                    const auto end = sigs[i].end();
                    auto p1 = begin;
                    while (p1 != end) {
                        // move p1 to the next non-zero interval's begin
                        while (p1 != end && *p1 == 0) ++p1;
                        if (p1 == end) break;
                        split_points[i].push_back(p1 - begin + sigs[i].getOffset());
                        // move p2 to the end of the current interval
                        auto p2 = p1;
                        while (p2 != end && *p2 != 0) ++p2;
                        // whether p2 equals end doesn't matter, just push the end
                        split_points[i].push_back(p2 - begin + sigs[i].getOffset() - 1);
                        // step p1 to p2, go to next loop
                        p1 = p2;
                    }
                }
                // check if the current box needs to be split or shrink
                bool split_or_shrink = false;
                for (auto i = 0; i < dim; ++i) {
                    if (split_points[i].size() > 2
                        || (split_points[i][0] > sigs[i].getOffset()
                            || split_points[i][1] < sigs[i].getOffset() + sigs[i].size() - 1))
                        split_or_shrink = true;
                }
                if (split_or_shrink) {
                    // construct all the possible compact boxes
                    DS::Range<dim> range;
                    for (auto i = 0; i < dim; ++i) {
                        range.start[i] = 0;
                        range.end[i] = split_points[i].size() / 2;
                    }
                    range.reValidPace();
                    rangeFor_s(range, [&](auto &&idx) {
                        // construct box
                        DS::Box<int, dim> t;
                        for (auto i = 0; i < dim; ++i) {
                            t.lo[i] = split_points[i][idx[i] * 2];
                            t.hi[i] = split_points[i][idx[i] * 2 + 1];
                        }
                        queue.push_back(t);
                    });
                    goto next_cycle;
                } else {
                    // the current box is already compact.
                    // we first deal with some corner cases
                    // case 1: the box is sufficient slim in one dim
                    std::array<bool, dim> slim_in_dim;
                    bool all_slim = true;
                    for (auto i = 0; i < dim; ++i) {
                        // 1, 2, 3 points thick are considered slim
                        slim_in_dim[i] = box.hi[i] - box.lo[i] < slim_threshold;
                        all_slim &= slim_in_dim[i];
                    }
                    // if the box is slim in all dims, accept it
                    if (all_slim) {
                        ret.push_back(box);
                        goto next_cycle;
                    }
                    // find the edge to split
                    // calculate the lap of sig
                    std::array<DS::OffsetVector<int>, dim> laps = sigs;
                    for (auto i = 0; i < dim; ++i) {
                        // for slim dims, we won't split
                        if (slim_in_dim[i]) continue;
                        for (auto j = laps[i].getOffset() + 1; j < laps[i].getOffset() + laps[i].size() - 1;
                             ++j) {
                            laps[i][j] = (sigs[i][j - 1] - 2 * sigs[i][j] + sigs[i][j + 1]);
                        }
                    }
                    // find all the sign-change points of lap
                    std::array<std::vector<int>, dim> zero_points;
                    std::array<int, dim> max_jumps;
                    max_jumps.fill(0);
                    for (auto i = 0; i < dim; ++i) {
                        for (auto j = box.lo[i] + 1; j <= box.hi[i] - 2; ++j) {
                            if ((laps[i][j] <= 0 && laps[i][j + 1] >= 0)
                                || (laps[i][j] >= 0 && laps[i][j + 1] <= 0)) {
                                // always take the max jump
                                if (std::abs(laps[i][j] - laps[i][j + 1]) > max_jumps[i]) {
                                    // clear the existing list and take the new one
                                    max_jumps[i] = std::abs(laps[i][j] - laps[i][j + 1]);
                                    zero_points[i].clear();
                                    zero_points[i].push_back(j);
                                } else if (std::abs(laps[i][j] - laps[i][j + 1]) == max_jumps[i]) {
                                    // append the current point
                                    zero_points[i].push_back(j);
                                }
                            }
                        }
                    }
                    std::array<int, dim> final_splits;
                    for (auto i = 0; i < dim; ++i) {
                        final_splits[i] = box.lo[i] - 1;
                        // take the most center split point
                        if (zero_points[i].size() == 0) continue;
                        else if (zero_points[i].size() == 1)
                            final_splits[i] = zero_points[i][0];
                        else {
                            auto median = (box.hi[i] + box.lo[i]) / 2;
                            final_splits[i] = zero_points[i][0];
                            for (auto &p : zero_points[i]) {
                                if (std::abs(final_splits[i] - median) > std::abs(p - median))
                                    final_splits[i] = p;
                            }
                        }
                    }
                    // sort the dims according to the max jumps. the dim with the max jumps
                    // is preferred for split
                    std::array<int, dim> split_priority;
                    for (auto i = 0; i < dim; ++i) split_priority[i] = i;
                    std::sort(split_priority.begin(), split_priority.end(),
                              [&](auto a, auto b) { return max_jumps[a] > max_jumps[b]; });
                    // try split the box according to the priority dims
                    for (auto i : split_priority) {
                        if (zero_points[i].size() > 0) {
                            // if the split won't generate slim boxes
                            if (final_splits[i] - box.lo[i] + 1 > slim_threshold
                                && box.hi[i] - final_splits[i] + 1 > slim_threshold) {
                                // do the split
                                auto box1 = box, box2 = box;
                                box1.hi[i] = final_splits[i];
                                box2.lo[i] = final_splits[i] + 1;
                                queue.push_back(box1);
                                queue.push_back(box2);
                                goto next_cycle;
                            } else {
                                // the split will make slim boxes, take the next dim for test
                                continue;
                            }
                        }
                    }

                    // if could reach here, any of the split plan will make slim boxes
                    // or they can't make slice at all. we then bisect along the longest dim
                    auto longest_dim = 0, length = 0;
                    for (auto i = 0; i < dim; ++i) {
                        if (box.hi[i] - box.lo[i] + 1 > length) {
                            length = box.hi[i] - box.lo[i] + 1;
                            longest_dim = i;
                        }
                    }
                    // bisect the box
                    auto box1 = box, box2 = box;
                    box1.hi[longest_dim] = (box.lo[longest_dim] + box.hi[longest_dim]) / 2;
                    box2.lo[longest_dim] = box1.hi[longest_dim] + 1;
                    queue.push_back(box1);
                    queue.push_back(box2);
                    goto next_cycle;
                }
            }
        next_cycle:
            continue;
        }
        // return all the generated boxes
        return ret;
    }

    void dump_points(auto &points, double h, const std::string &fname) {
        vtkNew<vtkPoints> vtkp;
        for (auto &p : points) { vtkp->InsertNextPoint((p[0] + 0.5) * h, (p[1] + 0.5) * h, 0); }
        vtkNew<vtkPolyData> polydata;
        polydata->SetPoints(vtkp);

        vtkNew<vtkXMLPolyDataWriter> writer;
        writer->SetFileName(fname.c_str());
        writer->SetInputData(polydata);
        writer->Write();
    }

    auto init_grid(auto &stack, int max_level, int buff_width, double th, auto &&func) {
        const auto &baseMesh = stack.levels.front().zones.front().baseMesh;
        constexpr static auto dim = Meta::RealType<decltype(baseMesh)>::dim;
        auto ratio = stack.ratio;
        for (auto level = max_level - 1; level >= 0; --level) {
            auto currentDims = baseMesh.dims;
            auto currentOffsets = baseMesh.offsets;
            // we fix the (0, 0, ...) index and scale the existing range
            for (auto i = 0; i < dim; ++i) {
                currentOffsets[i] *= int_pow(ratio, level);
                currentDims[i] = (currentDims[i] - 1) * int_pow(ratio, level);
            }
            using Point = DS::Point<int, dim>;
            std::vector<Point> points;
            DS::Range<dim> range;// cell centered range
            for (auto i = 0; i < dim; ++i) {
                range.start[i] = currentOffsets[i];
                range.end[i] = currentOffsets[i] + currentDims[i] - 1;
            }
            // label all the necessary cells on the current level
            rangeFor_s(range, [&](auto &&i) {
                if (func(level, i)) points.template emplace_back(i.get());
            });
#ifndef NDEBUG
            OP_DEBUG("Points at level {}:", level);
            for (const auto &p : points) { OP_DEBUG("{}", p.toString()); }
#endif

#ifndef NDEBUG
            std::vector<Point> p_add;
#endif
            // label all the points under level + 1
            if (level < max_level - 1) {
                for (auto &z : (std::next(stack.levels.begin()))->zones) {
                    auto _range = z.baseMesh.range;
                    // convert level l + 1 's range to level l - 1 's range
                    for (auto i = 0; i < dim; ++i) {
                        _range.start[i] = (_range.start[i] - buff_width * ratio) / (ratio * ratio);
                        _range.end[i] = (_range.end[i] + buff_width * ratio - 1 + ratio * ratio - 1)
                                        / (ratio * ratio);
                    }
                    // restrict _range
                    for (auto i = 0; i < dim; ++i) {
                        _range.start[i] = std::max(currentOffsets[i], _range.start[i]);
                        _range.end[i] = std::min(currentOffsets[i] + currentDims[i], _range.end[i]);
                    }
                    // label all points in _range
                    rangeFor_s(_range, [&](auto &&i) {
                        points.template emplace_back(i.get());
#ifndef NDEBUG
                        p_add.template emplace_back(i.get());
#endif
                    });
                }
            }
            // remove duplicates in points
            std::sort(points.begin(), points.end());
            auto last = std::unique(points.begin(), points.end());
            points.erase(last, points.end());
#ifndef NDEBUG
            OP_DEBUG("Points at level {}:", level);
            for (const auto &p : points) { OP_DEBUG("{}", p.toString()); }
            dump_points(points, 1. / int_pow(ratio, level), fmt::format("p_level{}.vtp", level));
            dump_points(p_add, 1. / int_pow(ratio, level), fmt::format("padd_level{}.vtp", level));
#endif
            auto tree = DS::KdTree<int, dim>(points);
            auto boxes = domainPartition(tree, th, 2);
#ifndef NDEBUG
            OP_DEBUG("Boxes at level {}:", level);
            for (auto &b : boxes) { OP_DEBUG("{}", b.toString()); }
#endif
            using Level = Meta::RealType<decltype(stack.levels.front())>;
            using Zone = typename Level::ZoneType;
            Level newLevel;
            auto newMesh = baseMesh.getRefinedMesh(int_pow(ratio, level + 1));
            // construct each zone in new level
            for (auto &box : boxes) {
                // convert box to next level's range
                DS::Range<dim> r;
                for (auto i = 0; i < dim; ++i) {
                    r.start[i] = ratio * box.lo[i];
                    r.end[i] = ratio * (box.hi[i] + 1) + 1;
                }
                r.reValidPace();
                // get submesh
                auto submesh = newMesh.getSubMesh(r);
                auto newZone = typename Zone::Builder().setMesh(submesh).build();
                newLevel.zones.push_back(newZone);
            }
            stack.levels.insert(std::next(stack.levels.begin()), std::move(newLevel));
        }
    }
}// namespace OpFlow
#endif//OPFLOW_AMRGEN_HPP
