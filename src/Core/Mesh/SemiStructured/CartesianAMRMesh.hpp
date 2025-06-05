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

#ifndef OPFLOW_CARTESIANAMRMESH_HPP
#define OPFLOW_CARTESIANAMRMESH_HPP

#include "Core/Loops/RangeFor.hpp"
#include "Core/Mesh/SemiStructured/CartesianAMRMeshBase.hpp"
#include "Core/Mesh/SemiStructured/CartesianAMRMeshView.hpp"
#include "Core/Mesh/Structured/CartesianMesh.hpp"
#include "DataStructures/Geometry/BasicElements.hpp"
#include "DataStructures/Geometry/KdTree.hpp"
#include "DataStructures/Index/LevelMDIndex.hpp"
#include "DataStructures/Range/LevelRanges.hpp"
#include "Math/Function/Numeric.hpp"
#ifndef OPFLOW_INSIDE_MODULE
#include <deque>
#include <map>
#include <vector>
#endif
#ifdef OPFLOW_WITH_VTK
#ifndef OPFLOW_INSIDE_MODULE
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkXMLPolyDataWriter.h>
#endif
#endif

namespace OpFlow {
    template <typename Dim>
    struct CartesianAMRMesh : CartesianAMRMeshBase<CartesianAMRMesh<Dim>> {
        static constexpr Size dim = Dim::value;

        // A CartesianAMRMesh (patch based) consists of a stack of Cartesian meshes.
        // On each level, there can be several parts across the entire
        // index space. All parts on the same level share the same
        // refinement ratio.
        std::vector<CartesianMesh<Dim>> meshes;
        std::vector<std::vector<DS::LevelRange<dim>>> ranges;
        std::vector<std::vector<std::vector<int>>> neighbors, parents;

        // Control parameters for AMR
        /// Fill rate threshold for remeshing
        Real fillRateThreshold = 0.6;
        /// Threshold for determine whether a box is slim.
        /// All result boxes should not be slimmer than this width in any dimension.
        int slimThreshold = 1;
        /// Max number of levels for refinement
        int maxLevel = 1;
        /// Scheme determined buffer zone's width
        int buffWidth = 0;
        /// The refinement ratio between levels. \note We assume all levels share
        /// the same refinement ratio in all dimensions. Althougn this is not a must for
        /// Cartesian based AMR schemes, it's a reasonable choice for common cases.
        /// If we do need a more general representation, we can re-design this parameter later.
        int refinementRatio = -1;

        CartesianAMRMesh() = default;

        explicit CartesianAMRMesh(const CartesianMesh<Dim> &baseMesh) {
            meshes.push_back(baseMesh);
            ranges.template emplace_back(baseMesh.getRange());
        }
        auto getPtr() const { return this; }

        auto getPtr() { return this; }

        auto &getRanges() const { return ranges; }
        auto getView() const { return CartesianAMRMeshView(*this); }

        auto x(int d, const DS::LevelMDIndex<dim> &i) const { return x(d, i.l, i[d]); }
        auto x(int d, int l, int i) const { return meshes[l].x(d, i); }
        auto dx(int d, const DS::LevelMDIndex<dim> &i) const { return dx(d, i.l, i[d]); }
        auto dx(int d, int l, int i) const { return meshes[l].dx(d, i); }
        auto idx(int d, const DS::LevelMDIndex<dim> &i) const { return idx(d, i.l, i[d]); }
        auto idx(int d, int l, int i) const { return meshes[l].idx(d, i); }

        bool operator==(const CartesianAMRMesh &other) const {
            return ranges == other.ranges && meshes == other.meshes;
        }
    };

    template <typename Dim>
    struct MeshBuilder<CartesianAMRMesh<Dim>> {
        static constexpr auto dim = Dim::value;
        CartesianAMRMesh<Dim> ret;
        const CartesianAMRMesh<Dim> *ref = nullptr;

        MeshBuilder() = default;
        auto &setRefMesh(const CartesianAMRMesh<Dim> &m_ref) {
            this->ref = &m_ref;
            ret = m_ref;
            return *this;
        }
        auto &setRefMesh(const CartesianAMRMesh<Dim> *m_ref) {
            this->ref = m_ref;
            ret = *ref;
            return *this;
        }

        auto &setBaseMesh(const CartesianMesh<Dim> &baseMesh) {
            ret = CartesianAMRMesh<Dim>();// clean up
            ret.meshes.push_back(baseMesh);
            ret.partRanges.emplace_back(baseMesh.getRange());
            return *this;
        }

        auto &setBaseMesh(CartesianMesh<Dim> &&baseMesh) {
            ret = CartesianAMRMesh<Dim>();// clean up
            ret.ranges.emplace_back();
            ret.ranges.back().template emplace_back(baseMesh.getRange());
            ret.meshes.push_back(std::move(baseMesh));
            return *this;
        }

        auto &setRefinementRatio(int r) {
            ret.refinementRatio = r;
            return *this;
        }

        auto &setFillRateThreshold(double fr) {
            ret.fillRateThreshold = fr;
            return *this;
        }

        auto &setSlimThreshold(int w) {
            ret.slimThreshold = w;
            return *this;
        }

        auto &setMaxLevel(int m) {
            ret.maxLevel = m;
            ret.meshes.resize(m);
            ret.ranges.resize(m);
            return *this;
        }

        auto &setBuffWidth(int w) {
            ret.buffWidth = w;
            return *this;
        }

        auto &setMarkerFunction(auto &&func) {
            marker = func;
            return *this;
        }

        auto build() {
            check();
            if (!ref) init_grid(marker);
            else
                init_grid_with_ref(marker);
            gen_relation();
            return ret;
        }

    private:
        std::function<bool(const DS::LevelMDIndex<dim> &)> marker;

        void check() {
            // We only have to check if the refinement ratio has been set
            OP_ASSERT(ret.refinementRatio > 0);
        }
        auto init_grid(auto &&func) {
            const auto &baseMesh = ret.meshes[0];
            auto ratio = ret.refinementRatio;
            for (auto _level = ret.maxLevel - 1; _level > 0; --_level) {
                auto currentDims = baseMesh.getDims();
                auto currentOffsets = baseMesh.getStart();
                // we fix the (0, 0, ...) index and scale the existing range
                for (auto i = 0; i < dim; ++i) {
                    currentOffsets[i] *= Math::int_pow(ratio, _level - 1);
                    currentDims[i] = (currentDims[i] - 1) * Math::int_pow(ratio, _level - 1);
                }
                using Point = DS::PointWithLabel<int, int, dim>;
                std::vector<Point> points;
                DS::LevelRange<dim> range;// cell centered range
                for (auto i = 0; i < dim; ++i) {
                    range.start[i] = currentOffsets[i];
                    range.end[i] = currentOffsets[i] + currentDims[i];
                }
                range.level = _level - 1;
                // label all the necessary cells on the current _level
                rangeFor_s(range, [&](auto &&i) {
                    if (func(i)) points.template emplace_back(i.get());
                });
#ifndef NDEBUG
                OP_DEBUG("Points at _level {}:", _level - 1);
                for (const auto &p : points) { OP_DEBUG("{}", p.toString()); }
#endif

#ifndef NDEBUG
                std::vector<Point> p_add;
#endif
                // label all the points under _level + 1
                if (_level < ret.maxLevel - 1) {
                    for (auto &r : ret.ranges[_level + 1]) {
                        auto _range = r;
                        // convert _level l + 1 's range to _level l - 1 's range
                        for (auto i = 0; i < dim; ++i) {
                            _range.start[i] = (_range.start[i] - ret.buffWidth * ratio) / (ratio * ratio);
                            _range.start[i]
                                    = std::max(_range.start[i],
                                               baseMesh.getStartOf(i) * Math::int_pow(ratio, _level - 1));
                            _range.end[i] = (_range.end[i] + ret.buffWidth * ratio - 1 + ratio * ratio - 1)
                                            / (ratio * ratio);
                            _range.end[i]
                                    = std::min(_range.end[i], +(baseMesh.getEndOf(i) - 1)
                                                                      * Math::int_pow(ratio, _level - 1));
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
                OP_DEBUG("Points at _level {}:", _level);
                for (const auto &p : points) { OP_DEBUG("{}", p.toString()); }
                dump_points(points, 1. / Math::int_pow(ratio, _level - 1),
                            std::format("p_level{}.vtp", _level));
                dump_points(p_add, 1. / Math::int_pow(ratio, _level - 1),
                            std::format("padd_level{}.vtp", _level));
#endif
                auto tree = MarkerTree(points);
                auto boxes = domainPartition(tree, ret.fillRateThreshold, ret.slimThreshold);
#ifndef NDEBUG
                OP_DEBUG("Boxes at _level {}:", _level);
                for (auto &b : boxes) { OP_DEBUG("{}", b.toString()); }
#endif

                auto newMesh = MeshBuilder<CartesianMesh<Meta::int_<dim>>>()
                                       .newMesh(baseMesh)
                                       .refine(Math::int_pow(ratio, _level))
                                       .build();
                ret.meshes[_level] = newMesh;
                ret.ranges[_level].clear();
                // construct each zone in new _level
                for (auto &box : boxes) {
                    // convert box to next _level's range
                    DS::LevelRange<dim> r;
                    for (auto i = 0; i < dim; ++i) {
                        r.start[i] = box.lo[i] * ratio;
                        r.end[i] = (box.hi[i] + 1) * ratio + 1;
                    }
                    r.reValidPace();
                    r.level = _level;
                    r.part = ret.ranges[_level].size();
                    ret.ranges[_level].push_back(r);
                }
            }
        }
        auto init_grid_with_ref(auto &&func) {
            const auto &baseMesh = ret.meshes[0];
            auto ratio = ret.refinementRatio;
            for (auto _level = ret.maxLevel - 1; _level > 0; --_level) {
                using Point = DS::PointWithLabel<int, int, dim>;
                std::vector<Point> points;

                for (auto p = 0; p < ref->ranges[_level - 1].size(); ++p) {
                    // for each accessible patch p
                    auto range = ref->ranges[_level - 1][p];
                    // convert range to cell centered range
                    for (auto i = 0; i < dim; ++i) range.end[i]--;
                    rangeFor_s(range, [&](auto &&i) {
                        if (func(i)) points.template emplace_back(i.get());
                    });
                }
#ifndef NDEBUG
                OP_DEBUG("Points at _level {}:", _level - 1);
                for (const auto &p : points) { OP_DEBUG("{}", p.toString()); }
#endif

#ifndef NDEBUG
                std::vector<Point> p_add;
#endif
                // label all the points under _level + 1
                if (_level < ret.maxLevel - 1) {
                    for (auto &r : ret.ranges[_level + 1]) {
                        auto _range = r;
                        // convert _level l + 1 's range to _level l - 1 's range
                        for (auto i = 0; i < dim; ++i) {
                            _range.start[i] = (_range.start[i] - ret.buffWidth * ratio) / (ratio * ratio);
                            _range.start[i]
                                    = std::max(_range.start[i],
                                               baseMesh.getStartOf(i) * Math::int_pow(ratio, _level - 1));
                            _range.end[i] = (_range.end[i] + ret.buffWidth * ratio - 1 + ratio * ratio - 1)
                                            / (ratio * ratio);
                            _range.end[i]
                                    = std::min(_range.end[i], +(baseMesh.getEndOf(i) - 1)
                                                                      * Math::int_pow(ratio, _level - 1));
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
                OP_DEBUG("Points at _level {}:", _level);
                for (const auto &p : points) { OP_DEBUG("{}", p.toString()); }
                dump_points(points, 1. / Math::int_pow(ratio, _level - 1),
                            std::format("p_level{}.vtp", _level));
                dump_points(p_add, 1. / Math::int_pow(ratio, _level - 1),
                            std::format("padd_level{}.vtp", _level));
#endif
                auto tree = MarkerTree(points);
                auto boxes = domainPartition(tree, ret.fillRateThreshold, ret.slimThreshold);
#ifndef NDEBUG
                OP_DEBUG("Boxes at _level {}:", _level);
                for (auto &b : boxes) { OP_DEBUG("{}", b.toString()); }
#endif

                auto newMesh = MeshBuilder<CartesianMesh<Meta::int_<dim>>>()
                                       .newMesh(baseMesh)
                                       .refine(Math::int_pow(ratio, _level))
                                       .build();
                ret.meshes[_level] = newMesh;
                ret.ranges[_level].clear();
                // construct each zone in new _level
                for (auto &box : boxes) {
                    // convert box to next _level's range
                    DS::LevelRange<dim> r;
                    for (auto i = 0; i < dim; ++i) {
                        r.start[i] = box.lo[i] * ratio;
                        r.end[i] = (box.hi[i] + 1) * ratio + 1;
                    }
                    r.reValidPace();
                    r.level = _level;
                    r.part = ret.ranges[_level].size();
                    ret.ranges[_level].push_back(r);
                }
            }
        }

        auto gen_relation() {
            ret.neighbors.resize(ret.ranges.size());
            ret.parents.resize(ret.ranges.size());
            for (auto l = ret.maxLevel - 1; l > 0; --l) {
                // neighbors
                ret.neighbors[l].resize(ret.ranges[l].size());
                for (auto &p : ret.neighbors[l]) p.clear();
                for (auto p = 0; p < ret.ranges[l].size(); ++p) {
                    for (auto pp = p + 1; pp < ret.ranges[l].size(); ++pp) {
                        auto ext = ret.ranges[l][p].getInnerRange(-ret.buffWidth);
                        if (DS::intersectRange(ext, ret.ranges[l][pp])) {
                            ret.neighbors[l][p].push_back(pp);
                            ret.neighbors[l][pp].push_back(p);
                        }
                    }
                }
                // parents
                ret.parents[l].resize(ret.ranges[l].size());
                for (auto &p : ret.parents[l]) p.clear();
                if (l - 1 >= 0) {
                    for (auto p = 0; p < ret.ranges[l].size(); ++p) {
                        for (auto pp = 0; pp < ret.ranges[l - 1].size(); ++pp) {
                            auto ext = ret.ranges[l][p].getInnerRange(-ret.buffWidth);
                            auto p_upcast = ret.ranges[l - 1][pp];
                            for (auto i = 0; i < dim; ++i) {
                                p_upcast.start[i] *= ret.refinementRatio;
                                p_upcast.end[i] *= ret.refinementRatio;
                            }
                            if (DS::intersectRange(ext, p_upcast)) { ret.parents[l][p].push_back(pp); }
                        }
                    }
                }
            }
        }

    private:
        using MarkerTree = DS::KdTree<DS::PointWithLabel<int, int, dim>, DS::Box<int, dim>>;
        using Box = DS::Box<int, dim>;

        static auto getFillRate(const MarkerTree &tree, const Box &box) {
            auto total = 1;
            for (auto i = 0; i < dim; ++i) { total *= (box.hi[i] - box.lo[i] + 1); }
            auto count = tree.countInBox(box);
            return (double) count / total;
        }

        static void dump_points(auto &points, double h, const std::string &fname) {
#ifdef OPFLOW_WITH_VTK
            vtkNew<vtkPoints> vtkp;
            for (auto &p : points) { vtkp->InsertNextPoint((p[0] + 0.5) * h, (p[1] + 0.5) * h, 0); }
            vtkNew<vtkPolyData> polydata;
            polydata->SetPoints(vtkp);

            vtkNew<vtkXMLPolyDataWriter> writer;
            writer->SetFileName(fname.c_str());
            writer->SetInputData(polydata);
            writer->Write();
#else
            OP_ERROR("MeshBuilder::dump_points not working because VTK is not enabled");
#endif
        }

        static auto domainPartition(const MarkerTree &marker, double fr_threshold, int slim_threshold) {
            std::deque<Box> queue;
            // find the global AABB and push it into queue
            queue.push_back(marker.boundingBox());
            // the result vector
            std::vector<OpFlow::DS::Box<int, dim>> ret;
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
                        DS::LevelRange<dim> range;
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
                            for (auto j = laps[i].getOffset() + 1;
                                 j < laps[i].getOffset() + laps[i].size() - 1; ++j) {
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
    };

    template <typename Dim>
    struct CartesianAMRMeshDirectComposer {
        static constexpr auto dim = Dim::value;
        CartesianAMRMesh<Dim> ret;

        CartesianAMRMeshDirectComposer() = default;
        auto &setBaseMesh(const CartesianMesh<Dim> &baseMesh) {
            ret = CartesianAMRMesh<Dim>();// clean up
            ret.meshes.push_back(baseMesh);
            ret.partRanges.emplace_back(baseMesh.getRange());
            return *this;
        }

        auto &setBaseMesh(CartesianMesh<Dim> &&baseMesh) {
            ret = CartesianAMRMesh<Dim>();// clean up
            ret.ranges.emplace_back();
            ret.ranges.back().template emplace_back(baseMesh.getRange());
            ret.meshes.push_back(std::move(baseMesh));
            return *this;
        }

        auto &setRefinementRatio(int r) {
            ret.refinementRatio = r;
            return *this;
        }

        auto &setBuffWidth(int w) {
            ret.buffWidth = w;
            return *this;
        }

        auto &addPatch(DS::LevelRange<dim> range) {
            if (range.level + 1 > ret.maxLevel) {
                ret.ranges.resize(range.level + 1);
                auto origin_size = ret.meshes.size();
                for (auto i = origin_size; i < range.level + 1; ++i) {
                    ret.meshes.push_back(MeshBuilder<CartesianMesh<Dim>>()
                                                 .newMesh(ret.meshes[0])
                                                 .refine(Math::int_pow(ret.refinementRatio, i))
                                                 .build());
                }
                ret.maxLevel = range.level + 1;
            }
            range.part = ret.ranges[range.level].size();
            ret.ranges[range.level].push_back(std::move(range));
            return *this;
        }

        auto build() {
            gen_relation();
            check();
            return ret;
        }

    private:
        void gen_relation() {
            ret.neighbors.resize(ret.ranges.size());
            ret.parents.resize(ret.ranges.size());
            for (auto l = ret.maxLevel - 1; l > 0; --l) {
                // neighbors
                ret.neighbors[l].resize(ret.ranges[l].size());
                for (auto &p : ret.neighbors[l]) p.clear();
                for (auto p = 0; p < ret.ranges[l].size(); ++p) {
                    for (auto pp = p + 1; pp < ret.ranges[l].size(); ++pp) {
                        auto ext = ret.ranges[l][p].getInnerRange(-ret.buffWidth);
                        if (DS::intersectRange(ext, ret.ranges[l][pp])) {
                            ret.neighbors[l][p].push_back(pp);
                            ret.neighbors[l][pp].push_back(p);
                        }
                    }
                }
                // parents
                ret.parents[l].resize(ret.ranges[l].size());
                for (auto &p : ret.parents[l]) p.clear();
                if (l - 1 >= 0) {
                    for (auto p = 0; p < ret.ranges[l].size(); ++p) {
                        for (auto pp = 0; pp < ret.ranges[l - 1].size(); ++pp) {
                            auto ext = ret.ranges[l][p].getInnerRange(-ret.buffWidth);
                            auto p_upcast = ret.ranges[l - 1][pp];
                            for (auto i = 0; i < dim; ++i) {
                                p_upcast.start[i] *= ret.refinementRatio;
                                p_upcast.end[i] *= ret.refinementRatio;
                            }
                            if (DS::intersectRange(ext, p_upcast)) { ret.parents[l][p].push_back(pp); }
                        }
                    }
                }
            }
        }
        void check() {
            // check if refinementRatio has been set
            OP_ASSERT(ret.refinementRatio > 0);
            // check if proper nesting is satisfied - divide & conquer
            for (auto l = 1; l < ret.maxLevel; ++l) {
                for (auto &r : ret.ranges[l]) {
                    // for each range r
                    // collect all parent ranges
                    std::vector<DS::LevelRange<dim>> pRanges;
                    for (auto p : ret.parents[l][r.part]) {
                        auto pRange = ret.ranges[l - 1][p];
                        // scale parent range to level l
                        for (auto d = 0; d < dim; ++d) {
                            pRange.start[d] *= ret.refinementRatio;
                            pRange.end[d] = (pRange.end[d] - 1) * ret.refinementRatio + 1;
                        }
                        pRange.level = l;
                        pRanges.push_back(pRange);
                    }
                    int dir = 0;
                    auto covered = dac_check_coverage(r, pRanges, dir);
                    if (!covered) {
                        OP_ERROR("Proper nesting not satisfied.");
                        OP_ABORT;
                    }
                }
            }
        }
        bool dac_check_coverage(const DS::LevelRange<dim> &r, const std::vector<DS::LevelRange<dim>> &pRanges,
                                int dir) {
            // check if the current range is already contained by one parent
            for (const auto &p : pRanges) {
                if (DS::inRange(r, p)) return true;
                else {
                    // if r only have a single element, then the algorithm has already failed
                    if (r.count() == 1) return false;
                    else {
                        // further divide r by dir
                        if (r.end[dir] - r.start[dir] == 1) {
                            // find next dividable dir
                            while (r.end[dir] - r.start[dir] == 1) dir = (dir + 1) % dim;
                        }
                        auto r1 = r, r2 = r;
                        r1.end[dir] = (r.end[dir] - r.start[dir]) / 2 + r.start[dir];
                        r2.start[dir] = r1.end[dir];
                        return dac_check_coverage(r1, pRanges, (dir + 1) % dim)
                               && dac_check_coverage(r2, pRanges, (dir + 1) % dim);
                    }
                }
            }
            return r.count() == 0;
        }
    };
}// namespace OpFlow

#endif//OPFLOW_CARTESIANAMRMESH_HPP
