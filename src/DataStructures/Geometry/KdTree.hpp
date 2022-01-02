// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2022 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_KDTREE_HPP
#define OPFLOW_KDTREE_HPP

#include "DataStructures/Geometry/BasicElements.hpp"

namespace OpFlow::DS {
    template <typename PointType, typename BoxType>
    struct KdTree {
        constexpr static auto dim = PointType::dim;

        KdTree() = default;

        ~KdTree() {
            if (root) delete root;
        }

        explicit KdTree(const std::vector<PointType> &points) { initFromPoints(points); }

        explicit KdTree(std::vector<PointType> &points) { initFromPoints(points); }

        void initFromPoints(const std::vector<PointType> &points) {
            auto copy = points;
            initFromPoints(copy);
        }

        void initFromPoints(std::vector<PointType> &points) { root = buildSubtree(points, 0); }

        void traverse(auto &&func) const { traverse_impl(std::forward<decltype(func)>(func), root); }

        void traverseInBox(const BoxType &box, auto &&func) const {
            traverseInBox_impl(box, std::forward<decltype(func)>(func), root);
        }

        auto getPoints() const {
            std::vector<PointType> ret;
            ret.reserve(count());
            traverse([&](auto &&p) { ret.push_back(p); });
            return ret;
        }

        auto getPointsInBox(const BoxType &box) const {
            std::vector<PointType> ret;
            traverseInBox(box, [&](auto &&p) { ret.push_back(p); });
            return ret;
        }

        auto count() const { return root ? root->count : 0; }

        auto countInBox(const BoxType &box) const {
            auto ret = 0;
            countInBox_impl(
                    box, [&](auto n) { ret += n->count; }, root);
            return ret;
        }

        auto boundingBox() const { return root ? root->boundingBox : BoxType(); }

        auto boundingBoxInBox(const BoxType &box) const {
            BoxType ret;
            int counter = 0;
            countInBox_impl(
                    box,
                    [&](auto n) {
                        if (counter++ == 0) {
                            ret = n->boundingBox;
                        } else {
                            ret = boxMerge(ret, n->boundingBox);
                        }
                    },
                    root);
            return ret;
        }

    private:
        struct Node {
            PointType p;
            int count, dir;
            BoxType boundingBox;
            Node *lc = nullptr, *rc = nullptr, *pre = nullptr;

            ~Node() {
                if (lc) delete lc;
                if (rc) delete rc;
            }

            auto isLeaf() const { return lc == nullptr && rc == nullptr; }
        };

        auto buildSubtree(std::vector<PointType> &points, int level) const {
            // special case: 0 point
            if (points.empty()) {
                return (Node *) nullptr;
            } else if (points.size() == 1) {
                auto ret = new Node();
                ret->p = points[0];
                ret->count = 1;
                ret->dir = level % dim;
                ret->boundingBox.lo = points[0].cord;
                ret->boundingBox.hi = points[0].cord;
                return ret;
            }
            auto mod_level = level % dim;
            auto op = [=](const auto &a, const auto &b) { return a[mod_level] < b[mod_level]; };
            std::stable_sort(points.begin(), points.end(), op);
            auto median = points.begin() + (points.size() - 1) / 2;
            auto pt = median, final = median;
            // special case:
            //     *   *
            //         *
            // the maximum[mod_level] == median[mod_level]. this will cause dead loop
            if ((*median)[mod_level] == points.back()[mod_level]) {
                if (points.front()[mod_level] != points.back()[mod_level]) {
                    // there exists smaller points. use the backward strategy
                    while (pt != points.begin()) {
                        if ((*pt)[mod_level] == (*median)[mod_level]) --pt;
                        else
                            break;
                    }
                    final = pt++;
                } else {
                    // all points shares the line. just take pt = end
                    pt = points.end();
                    final = pt - 1;
                }
            } else {
                // normal case. use the forward strategy
                while (pt != points.end()) {
                    if ((*pt)[mod_level] == (*median)[mod_level]) ++pt;
                    else
                        break;
                }
                final = pt - 1;
            }

            std::vector<PointType> left(points.begin(), pt), right(pt, points.end());
            // note: the containing relationship assures that the upper edges of each subzone
            // are included and the lower edges are not. This means that the left child will
            // always be valid (at least contains the current split point)
            auto lc = buildSubtree(left, level + 1);
            auto rc = buildSubtree(right, level + 1);
            // merge meta infos from lc & rc
            auto ret = new Node();
            ret->p = *final;
            ret->count = lc->count + (rc ? rc->count : 0);
            ret->dir = mod_level;
            ret->lc = lc;
            ret->rc = rc;
            ret->lc->pre = ret;
            if (rc) ret->rc->pre = ret;
            if (!rc) ret->boundingBox = ret->lc->boundingBox;
            else
                ret->boundingBox = boxMerge(ret->lc->boundingBox, ret->rc->boundingBox);
            return ret;
        }

        void traverse_impl(auto &&func, const Node *n) const {
            if (n) {
                if (n->isLeaf()) {
                    func(n->p);
                    return;
                }
                traverse_impl(std::forward<decltype(func)>(func), n->lc);
                traverse_impl(std::forward<decltype(func)>(func), n->rc);
            }
        }

        void traverseInBox_impl(const BoxType &box, auto &&func, const Node *base) const {
            if (!base) return;
            if (base->isLeaf()) {
                if (pointInBox(base->p, box)) { func(base->p); }
                return;
            } else {
                if (boxInBox(base->lc->boundingBox, box))
                    // report the entire left tree
                    traverse_impl(std::forward<decltype(func)>(func), base->lc);
                else if (boxIntersectBox(base->lc->boundingBox, box))
                    // recurse in left tree
                    traverseInBox_impl(box, std::forward<decltype(func)>(func), base->lc);

                if (!base->rc) return;
                if (boxInBox(base->rc->boundingBox, box))
                    // report the entire right tree
                    traverse_impl(std::forward<decltype(func)>(func), base->rc);
                else if (boxIntersectBox(base->rc->boundingBox, box))
                    // recurse in right tree
                    traverseInBox_impl(box, std::forward<decltype(func)>(func), base->rc);
            }
        }

        void countInBox_impl(const BoxType &box, auto &&counter, const Node *base) const {
            if (!base) return;
            if (base->isLeaf()) {
                if (pointInBox(base->p, box)) { counter(base); }
            } else {
                if (boxInBox(base->lc->boundingBox, box)) counter(base->lc);
                else if (boxIntersectBox(base->lc->boundingBox, box))
                    countInBox_impl(box, std::forward<decltype(counter)>(counter), base->lc);

                if (!base->rc) return;
                if (boxInBox(base->rc->boundingBox, box)) counter(base->rc);
                else if (boxIntersectBox(base->rc->boundingBox, box))
                    countInBox_impl(box, std::forward<decltype(counter)>(counter), base->rc);
            }
        }

        Node *root = nullptr;
    };
}// namespace OpFlow::DS
#endif//OPFLOW_KDTREE_HPP
