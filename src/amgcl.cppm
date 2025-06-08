module;

#define AMGCL_NO_BOOST

#include <amgcl/adapter/zero_copy.hpp>
#include <amgcl/profiler.hpp>
#include <amgcl/make_solver.hpp>
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

#ifdef OPFLOW_WITH_MPI
#include <amgcl/mpi/distributed_matrix.hpp>
#include <amgcl/mpi/make_solver.hpp>
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
#include <mpi.h>
#endif

#if defined(AMGCL_HAVE_PARMETIS)
#include <amgcl/mpi/partition/parmetis.hpp>
#elif defined(AMGCL_HAVE_SCOTCH)
#include <amgcl/mpi/partition/ptscotch.hpp>
#endif

export module ext.amgcl;

export namespace amgcl {
    namespace adapter {
        using amgcl::adapter::zero_copy;
    }
    using amgcl::profiler;
    namespace backend {
        using amgcl::backend::builtin;
        using amgcl::backend::sort_rows;
        using amgcl::backend::rows;
        using amgcl::backend::cols;
        using amgcl::backend::transpose;
        using amgcl::backend::nonzeros;
        using amgcl::backend::product;
        using amgcl::backend::diagonal;
    }
    using amgcl::make_solver;
    using amgcl::amg;
    namespace coarsening {
        using amgcl::coarsening::smoothed_aggregation;
    }
    namespace relaxation {
        using amgcl::relaxation::spai0;
    }
    namespace solver {
        using amgcl::solver::bicgstab;
        using amgcl::solver::bicgstabl;
        using amgcl::solver::cg;
        using amgcl::solver::fgmres;
        using amgcl::solver::gmres;
        using amgcl::solver::preonly;
        using amgcl::solver::skyline_lu;
    }
}