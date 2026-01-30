module;

#define AMGCL_NO_BOOST

#include <amgcl/adapter/zero_copy.hpp>
#include <amgcl/profiler.hpp>

#ifdef OPFLOW_WITH_MPI
#include <amgcl/mpi/util.hpp>
#include <amgcl/mpi/distributed_matrix.hpp>
#endif

export module ext.amgcl;

export namespace amgcl {
    namespace adapter {
        using amgcl::adapter::zero_copy;
    }
    using amgcl::profiler;
#ifdef OPFLOW_WITH_MPI
    namespace mpi {
        using amgcl::mpi::communicator;
        using amgcl::mpi::distributed_matrix;
    }
#endif
}
