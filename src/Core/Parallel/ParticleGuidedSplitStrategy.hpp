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

#ifndef OPFLOW_PARTICLEGUIDEDSPLITSTRATEGY_HPP
#define OPFLOW_PARTICLEGUIDEDSPLITSTRATEGY_HPP

#include "Core/Field/MeshBased/Structured/CartesianFieldTrait.hpp"
#include "Core/Parallel/AbstractSplitStrategy.hpp"
#ifdef OPFLOW_WITH_MPI
#include <mpi.h>
#endif
#include <array>
#include <bit>
#include <string>
#include <vector>

namespace OpFlow {
    template <int d>
    struct Particle {
        std::array<double, d> x;
    };

    template <CartesianFieldType F>
    struct ParticleGuidedSplitStrategy : AbstractSplitStrategy<F> {
    public:
        static constexpr int dim = internal::ExprTrait<F>::dim;

        ParticleGuidedSplitStrategy() = default;
        ParticleGuidedSplitStrategy(std::vector<Particle<dim>> parts, double part_load, double mesh_laod,
                                    int max_levels,
                                    const typename internal::ExprTrait<F>::mesh_type& ref_mesh)
            : particles(parts), part_load_factor(part_load), mesh_load_factor(mesh_laod),
              max_levels(max_levels), ref_mesh(ref_mesh) {}

        [[nodiscard]] std::string strategyName() const override { return "Particle guided split"; }
        typename internal::ExprTrait<F>::range_type
        splitRange(const typename internal::ExprTrait<F>::range_type& range,
                   const ParallelPlan& plan) override {
            check_state(plan);
            return split_impl(range, plan);
        }

        std::vector<typename internal::ExprTrait<F>::range_type>
        getSplitMap(const typename internal::ExprTrait<F>::range_type& range,
                    const ParallelPlan& plan) override {
            check_state(plan);
            return splitMap_impl(range, plan);
        }

        void setParticles(std::vector<Particle<dim>> parts) { particles = std::move(parts); }

        void setParticleLoad(double load) { part_load_factor = load; }

        void setParticleWeight(double w) { local_particle_weight = w; }

        void setMeshLoad(double load) { mesh_load_factor = load; }

        void setRefMesh(const typename internal::ExprTrait<F>::mesh_type& mesh) { ref_mesh = mesh; }

        void setMaxLevel(int level) { max_levels = level; }

    private:
        std::vector<Particle<dim>> particles;
        double local_particle_weight = 1;
        std::vector<double> particle_weight;
        typename internal::ExprTrait<F>::mesh_type ref_mesh;
        double part_load_factor = 0, mesh_load_factor = 1;
        int max_levels = 0;

        struct Node {
            std::unique_ptr<Node> lc, rc;
            Node* parent = nullptr;
            DS::Range<dim> range;
            std::vector<Particle<dim>> particles;
            std::vector<double> particle_weight;
            int split_axis = -1;// -1 for leaf node

            Node() = default;
            void traverse(auto&& func) const {
                if (!is_leaf()) {
                    lc->traverse(OP_PERFECT_FOWD(func));
                    rc->traverse(OP_PERFECT_FOWD(func));
                } else {
                    func(*this);
                }
            }

            void print_tree(std::string prefix) const {
                OP_INFO("{} range = {}, axis = {}", prefix, range.toString(), split_axis);
                if (!is_leaf()) {
                    prefix += "--";
                    OP_INFO("{} lc:", prefix);
                    lc->print_tree(prefix);
                    OP_INFO("{} rc:", prefix);
                    rc->print_tree(prefix);
                }
            }
            void splitAt(int d, int pos, const typename internal::ExprTrait<F>::mesh_type& mesh) {
                auto new_lc = std::make_unique<Node>();
                auto new_rc = std::make_unique<Node>();
                new_lc->range = range;
                new_lc->range.end[d] = pos + range.start[d];
                new_rc->range = range;
                new_rc->range.start[d] = pos + range.start[d];
                OP_ASSERT_MSG(!new_lc->range.empty() && !new_rc->range.empty(),
                              "split range {} at {} in dim {} results in empty range", range.toString(), pos,
                              d);
                new_lc->parent = this;
                new_rc->parent = this;
                split_axis = d;
                for (int i = 0; i < particles.size(); ++i) {
                    if (particles[i].x[d] < mesh.x(d, pos + range.start[d])) {
                        new_lc->particles.push_back(particles[i]);
                        new_lc->particle_weight.push_back(particle_weight[i]);
                    } else {
                        new_rc->particles.push_back(particles[i]);
                        new_rc->particle_weight.push_back(particle_weight[i]);
                    }
                }
                lc = std::move(new_lc);
                rc = std::move(new_rc);
            }

            void splitAtLoadMid(int d, double pload, double mload,
                                const typename internal::ExprTrait<F>::mesh_type& mesh) {
                auto offset = get_load_mid(d, pload, mload, mesh);
                splitAt(d, offset, mesh);
            }

            void is_splittable_at(int d, int pos) const { return range.start[d] < pos && pos < range.end[d]; }

            bool is_root() const { return !parent; }
            bool is_leaf() const { return !lc && !rc; }

            int get_load_mid(int d, double pload, double mload,
                             const typename internal::ExprTrait<F>::mesh_type& mesh) {
                auto load = get_load_along_dim(d, pload, mload, mesh);
                auto total_load = get_total_load(pload, mload);
                auto mid_iter = std::find_if_not(load.begin(), load.end(),
                                                 [=](auto&& l) { return l < total_load / 2; });
                return mid_iter - load.begin();
            }

            double get_total_load(double pload, double mload) const {
                double weighted_particle_size
                        = std::accumulate(particle_weight.begin(), particle_weight.end(), 0);
                return weighted_particle_size * pload + range.count() * mload;
            }

            std::vector<double> get_load_along_dim(int d, double pload, double mload,
                                                   const typename internal::ExprTrait<F>::mesh_type& mesh) {
                auto per_slice_load = get_per_slice_load_along_dim(d, pload, mload, mesh);
                std::vector<double> ret(per_slice_load.size());
                std::exclusive_scan(per_slice_load.begin(), per_slice_load.end(), ret.begin(), 0);
                return ret;
            }

            std::vector<double>
            get_per_slice_load_along_dim(int d, double pload, double mload,
                                         const typename internal::ExprTrait<F>::mesh_type& mesh) {
                std::vector<double> ret(range.end[d] - range.start[d], 0);
                if (!is_leaf()) {
                    if (d == split_axis) {
                        // merge results from children
                        auto left = lc->get_per_slice_load_along_dim(d, pload, mload, mesh);
                        auto right = rc->get_per_slice_load_along_dim(d, pload, mload, mesh);
                        std::copy(ret.begin(), ret.begin() + left.size(), left.begin());
                        std::copy(ret.begin() + left.size(), ret.end(), right.begin());
                        return ret;
                    } else {
                        // merge results from children by add
                        auto left = lc->get_per_slice_load_along_dim(d, pload, mload, mesh);
                        auto right = rc->get_per_slice_load_along_dim(d, pload, mload, mesh);
                        for (int i = 0; i < ret.size(); ++i) ret[i] = left[i] + right[i];
                        return ret;
                    }
                } else {
                    // do the real work
                    // sort particles along axis d
                    std::vector<std::pair<Particle<dim>, double>> buffer;
                    for (int i = 0; i < particles.size(); ++i) {
                        buffer.emplace_back(particles[i], particle_weight[i]);
                    }
                    std::sort(buffer.begin(), buffer.end(),
                              [d](auto&& a, auto&& b) { return a.first.x[d] < b.first.x[d]; });
                    for (int i = 0; i < particles.size(); ++i) {
                        particles[i] = buffer[i].first;
                        particle_weight[i] = buffer[i].second;
                    }
                    buffer.clear();
                    // add particle work load first
                    auto part_iter = particles.begin();
                    for (int i = 0; i < ret.size(); ++i) {
                        double xleft = mesh.x(d, i), xright = mesh.x(d, i + 1);
                        while (part_iter != particles.end() && xleft <= part_iter->x[d]
                               && part_iter->x[d] < xright) {
                            ret[i] += particle_weight[part_iter - particles.begin()] * pload;
                            part_iter++;
                        }
                    }
                    // then add mesh load
                    int slice_count = range.count() / ret.size();
                    for (auto& r : ret) r += slice_count * mload;
                    return ret;
                }
            }
        };

        std::unique_ptr<Node> kdtree = nullptr;

        void check_state(const ParallelPlan& plan) const {
            OP_ASSERT_MSG(1 << max_levels == plan.distributed_workers_count,
                          "ParticleGuidedSplitStrategy: total number of ranks must be power of 2");
        }

        void gather_all_particles() {
#if defined(OPFLOW_WITH_MPI)
            // gather counts
            std::vector<int> counts(getWorkerCount(), 0);
            int local_count = particles.size();
            MPI_Allgather(&local_count, 1, MPI_INT, counts.data(), 1, MPI_INT, MPI_COMM_WORLD);
            int total_part_size = std::accumulate(counts.begin(), counts.end(), 0);
            // gather weights
            std::vector<double> global_weights, weights_buffer(counts.size());
            MPI_Allgather(&local_particle_weight, 1, MPI_DOUBLE, weights_buffer.data(), 1, MPI_DOUBLE,
                          MPI_COMM_WORLD);
            global_weights.reserve(total_part_size);
            for (int i = 0; i < counts.size(); ++i) {
                for (int j = 0; j < counts[i]; ++j) global_weights.push_back(weights_buffer[i]);
            }
            particle_weight = std::move(global_weights);
            // gather parts
            std::vector<Particle<dim>> global_parts(total_part_size);
            std::vector<int> offsets(counts.size());
            std::exclusive_scan(counts.begin(), counts.end(), offsets.begin(), 0);
            for (auto& c : counts) c *= sizeof(Particle<dim>);
            for (auto& o : offsets) o *= sizeof(Particle<dim>);
            MPI_Allgatherv(particles.data(), local_count * sizeof(Particle<dim>), MPI_CHAR,
                           global_parts.data(), counts.data(), offsets.data(), MPI_CHAR, MPI_COMM_WORLD);
            particles = std::move(global_parts);
#else
            particle_weight.resize(particles.size());
            std::fill(particle_weight.begin(), particle_weight.end(), local_particle_weight);
#endif
        }

        // build a kd-tree of max_levels according to given particles and background mesh
        auto gen_split_tree(const DS::Range<dim>& range, const ParallelPlan& plan) {
            gather_all_particles();
            Node root;
            root.range = range;
            root.particles = particles;
            root.particle_weight = particle_weight;
            std::deque<Node*> build_queue;
            build_queue.push_back(&root);
            for (int i = 0; i < max_levels; ++i) {
                auto iter = build_queue.begin();
                auto end = build_queue.end();
                while (iter != end) {
                    auto ptr = *iter;
                    ptr->splitAtLoadMid(i % dim, part_load_factor, mesh_load_factor, ref_mesh);
                    build_queue.pop_front();
                    build_queue.push_back(ptr->lc.get());
                    build_queue.push_back(ptr->rc.get());
                    iter++;
                }
            }
            return std::make_unique<Node>(std::move(root));
        }

        auto split_impl(const DS::Range<dim>& range, const ParallelPlan& plan) {
            // assume the input range is nodal range, convert to central range
            auto _range = range;
            for (std::size_t i = 0; i < dim; ++i) _range.end[i]--;
            if (plan.singleNodeMode() || max_levels == 0) return _range;
            else {
                if (!kdtree) kdtree = gen_split_tree(_range, plan);
                auto* ptr = kdtree.get();
                // root is a full binary tree
                auto mask = OpFlow::getWorkerCount();
                auto rank = OpFlow::getWorkerId();
                OP_ASSERT_MSG(std::exp2((int) std::log2(mask)) == mask,
                              "total worker count must be power of 2");
                while (mask >>= 1) {
                    if (mask & rank) ptr = ptr->rc.get();
                    else
                        ptr = ptr->lc.get();
                }
                return ptr->range;
            }
        }

        auto splitMap_impl(const DS::Range<dim>& range, const ParallelPlan& plan) {
            // assume the input range is nodal range, convert to central range
            auto _range = range;
            for (std::size_t i = 0; i < dim; ++i) _range.end[i]--;
            if (plan.singleNodeMode()) return std::vector<DS::Range<dim>> {_range};
            else {
                if (!kdtree) kdtree = gen_split_tree(_range, plan);
#if defined(OPFLOW_WITH_MPI) || defined(OPFLOW_TEST_ENVIRONMENT)
                std::vector<DS::Range<dim>> ret;
                kdtree->traverse([&](const Node& node) { ret.push_back(node.range); });
                return ret;
#else
                OP_ERROR("Distributed parallel mode enabled without MPI library. Abort.");
                OP_ABORT;
#endif
            }
        }
    };
}// namespace OpFlow

#endif//OPFLOW_PARTICLEGUIDEDSPLITSTRATEGY_HPP
