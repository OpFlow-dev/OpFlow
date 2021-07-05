#ifndef OPFLOW_PARALLELPLAN_HPP
#define OPFLOW_PARALLELPLAN_HPP

#include <vector>

namespace OpFlow {
    enum class ParallelType : unsigned int { OpenMP = 0x1, MPI = 0x2, CUDA = 0x4 };

    struct ParallelPlan {
        ParallelType parallelType;
        int thread_per_proc;
        std::vector<int> dim_split;
    };
}// namespace OpFlow
#endif//OPFLOW_PARALLELPLAN_HPP
