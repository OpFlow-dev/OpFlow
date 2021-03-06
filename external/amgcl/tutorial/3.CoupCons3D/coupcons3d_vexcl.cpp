#include <iostream>
#include <vector>

#include <amgcl/adapter/block_matrix.hpp>
#include <amgcl/adapter/crs_tuple.hpp>
#include <amgcl/amg.hpp>
#include <amgcl/backend/vexcl.hpp>
#include <amgcl/backend/vexcl_static_matrix.hpp>
#include <amgcl/coarsening/smoothed_aggregation.hpp>
#include <amgcl/make_solver.hpp>
#include <amgcl/relaxation/ilu0.hpp>
#include <amgcl/solver/bicgstab.hpp>
#include <amgcl/value_type/static_matrix.hpp>

#include <amgcl/io/mm.hpp>
#include <amgcl/profiler.hpp>

int main(int argc, char *argv[]) {
    // The command line should contain the matrix file name:
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <matrix.mtx>" << std::endl;
        return 1;
    }

    // Create VexCL context. Set the environment variable OCL_DEVICE to
    // control which GPU to use in case multiple are available,
    // and use single device:
    vex::Context ctx(vex::Filter::Env && vex::Filter::Count(1));
    std::cout << ctx << std::endl;

    // Enable support for block-valued matrices in VexCL kernels:
    vex::scoped_program_header h1(ctx, amgcl::backend::vexcl_static_matrix_declaration<double, 4>());
    vex::scoped_program_header h2(ctx, amgcl::backend::vexcl_static_matrix_declaration<float, 4>());

    // The profiler:
    amgcl::profiler<> prof("Serena");

    // Read the system matrix:
    ptrdiff_t rows, cols;
    std::vector<ptrdiff_t> ptr, col;
    std::vector<double> val;

    prof.tic("read");
    std::tie(rows, cols) = amgcl::io::mm_reader(argv[1])(ptr, col, val);
    std::cout << "Matrix " << argv[1] << ": " << rows << "x" << cols << std::endl;
    prof.toc("read");

    // The RHS is filled with ones:
    std::vector<double> f(rows, 1.0);

    // Scale the matrix so that it has the unit diagonal.
    // First, find the diagonal values:
    std::vector<double> D(rows, 1.0);
    for (ptrdiff_t i = 0; i < rows; ++i) {
        for (ptrdiff_t j = ptr[i], e = ptr[i + 1]; j < e; ++j) {
            if (col[j] == i) {
                D[i] = 1 / sqrt(val[j]);
                break;
            }
        }
    }

    // Then, apply the scaling in-place:
    for (ptrdiff_t i = 0; i < rows; ++i) {
        for (ptrdiff_t j = ptr[i], e = ptr[i + 1]; j < e; ++j) { val[j] *= D[i] * D[col[j]]; }
        f[i] *= D[i];
    }

    // We use the tuple of CRS arrays to represent the system matrix.
    // Note that std::tie creates a tuple of references, so no data is actually
    // copied here:
    auto A = std::tie(rows, ptr, col, val);

    // Compose the solver type
    typedef amgcl::static_matrix<double, 4, 4> dmat_type;// matrix value type in double precision
    typedef amgcl::static_matrix<double, 4, 1> dvec_type;// the corresponding vector value type
    typedef amgcl::static_matrix<float, 4, 4> smat_type; // matrix value type in single precision

    typedef amgcl::backend::vexcl<dmat_type> SBackend;// the solver backend
    typedef amgcl::backend::vexcl<smat_type> PBackend;// the preconditioner backend

    typedef amgcl::make_solver<
            amgcl::amg<PBackend, amgcl::coarsening::smoothed_aggregation, amgcl::relaxation::ilu0>,
            amgcl::solver::bicgstab<SBackend>>
            Solver;

    // Solver parameters.
    //
    // The ILU0 relaxation is a serial algorithm. On the GPU the solutions of
    // the lower and upper parts of the incomplete LU decomposition are
    // approximated with several Jacobi iterations [1].
    //
    // Here we set the number of iterations as small as possible without
    // increasing the total number of iterations.
    //
    // [1] Chow, Edmond, and Aftab Patel. "Fine-grained parallel incomplete LU
    //     factorization." SIAM journal on Scientific Computing 37.2 (2015):
    //     C169-C193.
    Solver::params prm;
    prm.precond.relax.solve.iters = 4;

    // Set the VexCL context in the backend parameters
    SBackend::params bprm;
    bprm.q = ctx;

    // Initialize the solver with the system matrix.
    // We use the block_matrix adapter to convert the matrix into the block
    // format on the fly:
    prof.tic("setup");
    auto Ab = amgcl::adapter::block_matrix<dmat_type>(A);
    Solver solve(Ab, prm, bprm);
    prof.toc("setup");

    // Show the mini-report on the constructed solver:
    std::cout << solve << std::endl;

    // Solve the system with the zero initial approximation:
    int iters;
    double error;
    std::vector<double> x(rows, 0.0);

    // Since we are using mixed precision, we have to transfer the system matrix to the GPU:
    prof.tic("GPU matrix");
    auto A_gpu = SBackend::copy_matrix(std::make_shared<amgcl::backend::crs<dmat_type>>(Ab), bprm);
    prof.toc("GPU matrix");

    // We reinterpret both the RHS and the solution vectors as block-valued,
    // and copy them to the VexCL vectors:
    auto f_ptr = reinterpret_cast<dvec_type *>(f.data());
    auto x_ptr = reinterpret_cast<dvec_type *>(x.data());
    vex::vector<dvec_type> F(ctx, rows / 4, f_ptr);
    vex::vector<dvec_type> X(ctx, rows / 4, x_ptr);

    prof.tic("solve");
    std::tie(iters, error) = solve(*A_gpu, F, X);
    prof.toc("solve");

    // Output the number of iterations, the relative error,
    // and the profiling data:
    std::cout << "Iters: " << iters << std::endl << "Error: " << error << std::endl << prof << std::endl;
}
