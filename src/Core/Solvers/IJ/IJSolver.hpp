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

#ifndef OPFLOW_IJSOLVER_HPP
#define OPFLOW_IJSOLVER_HPP

#include <optional>

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

#include <amgcl/adapter/crs_tuple.hpp>
#include <amgcl/backend/builtin.hpp>

#ifdef OPFLOW_WITH_MPI
#include <mpi.h>
#include <amgcl/mpi/amg.hpp>
#include <amgcl/mpi/coarsening/smoothed_aggregation.hpp>
#include <amgcl/mpi/distributed_matrix.hpp>
#include <amgcl/mpi/make_solver.hpp>
#include <amgcl/mpi/relaxation/spai0.hpp>
#include <amgcl/mpi/solver/bicgstab.hpp>
#include <amgcl/mpi/solver/bicgstabl.hpp>
#include <amgcl/mpi/solver/cg.hpp>
#include <amgcl/mpi/solver/fgmres.hpp>
#include <amgcl/mpi/solver/gmres.hpp>
#include <amgcl/mpi/solver/preonly.hpp>
#endif

#include <amgcl/amg.hpp>
#include <amgcl/coarsening/smoothed_aggregation.hpp>
#include <amgcl/make_solver.hpp>
#include <amgcl/relaxation/spai0.hpp>
#include <amgcl/solver/bicgstab.hpp>
#include <amgcl/solver/bicgstabl.hpp>
#include <amgcl/solver/cg.hpp>
#include <amgcl/solver/fgmres.hpp>
#include <amgcl/solver/gmres.hpp>
#include <amgcl/solver/preonly.hpp>
#include <amgcl/solver/skyline_lu.hpp>

#include <amgcl/adapter/zero_copy.hpp>
#include <amgcl/io/binary.hpp>
#include <amgcl/profiler.hpp>

#if defined(AMGCL_HAVE_PARMETIS)
#include <amgcl/mpi/partition/parmetis.hpp>
#elif defined(AMGCL_HAVE_SCOTCH)
#include <amgcl/mpi/partition/ptscotch.hpp>
#endif

namespace OpFlow {
    template <typename Solver>
    struct IJSolverParams {
        typename Solver::params p;
        typename Solver::backend_params bp;
        bool staticMat = false, pinValue = false, verbose = false;
        std::optional<std::string> dumpPath {};
    };
}// namespace OpFlow

#endif//OPFLOW_IJSOLVER_HPP
