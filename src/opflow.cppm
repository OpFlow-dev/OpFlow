module;

#define AMGCL_NO_BOOST
#include <HYPRE.h>
#include <HYPRE_sstruct_ls.h>
#include <HYPRE_sstruct_mv.h>
#include <TECIO.h>
#include <amgcl/adapter/zero_copy.hpp>
#include <amgcl/profiler.hpp>
#include <bits/stdc++.h>
#include <oneapi/tbb.h>
#include <oneapi/tbb/detail/_range_common.h>
#include <spdlog/spdlog.h>
#ifdef OPFLOW_WITH_MPI
#include <amgcl/mpi/distributed_matrix.hpp>
#include <amgcl/mpi/make_solver.hpp>
#endif

export module opflow;

#define OPFLOW_INSIDE_MODULE

#include "OpFlow"