#include "SSHypre.hpp"
#include "fmt/format.h"
#include <HYPRE.h>
#include <HYPRE_sstruct_ls.h>
#include <HYPRE_sstruct_mv.h>
#include <iostream>

void sshypre_d2() {
    // step 1: gen composite grid
    /*     (1, 1) -> (4, 4) coarse mesh
     *     _________________________
     *     |     |     |__|__|__|__|
     *     |_____|_____|__|__|__|__|    (6, 6) -> (9, 9) fine mesh
     *     |     |     |__|__|__|__|
     *     |_____|_____|__|__|__|__|
     *     |     |     |     |     |
     *     |_____|_____|_____|_____|
     *     |     |     |     |     |
     *     |     |     |     |     |
     *     -------------------------
     * */

    HYPRE_SStructGrid grid;
    int ndim = 2, nparts = 2, nvars = 1, part = 0;
    int extends[][2] = {{1, 1}, {4, 4}, {6, 6}, {9, 9}};
    int vartypes[] = {HYPRE_SSTRUCT_VARIABLE_CELL};
    HYPRE_SStructGridCreate(MPI_COMM_WORLD, ndim, nparts, &grid);
    HYPRE_SStructGridSetExtents(grid, part, extends[0], extends[1]);
    HYPRE_SStructGridSetVariables(grid, part, nvars, vartypes);
    part = 1;
    HYPRE_SStructGridSetExtents(grid, part, extends[2], extends[3]);
    HYPRE_SStructGridSetVariables(grid, part, nvars, vartypes);
    HYPRE_SStructGridAssemble(grid);

    // step 2: set up the stencil
    /*          3
     *          *
     *          |
     *   2 * -- * -- * 1
     *          |
     *          *
     *          4
     * */
    HYPRE_SStructGraph graph;
    HYPRE_SStructStencil stencil;
    HYPRE_SStructStencilCreate(ndim, 5, &stencil);
    int c_var = 0, offset[][2] = {{0, 0}, {1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    HYPRE_SStructStencilSetEntry(stencil, 0, offset[0], c_var);
    HYPRE_SStructStencilSetEntry(stencil, 1, offset[1], c_var);
    HYPRE_SStructStencilSetEntry(stencil, 2, offset[2], c_var);
    HYPRE_SStructStencilSetEntry(stencil, 3, offset[3], c_var);
    HYPRE_SStructStencilSetEntry(stencil, 4, offset[4], c_var);
    HYPRE_SStructGraphCreate(MPI_COMM_WORLD, grid, &graph);
    HYPRE_SStructGraphSetStencil(graph, 0, c_var, stencil);
    HYPRE_SStructGraphSetStencil(graph, 1, c_var, stencil);
    // fine to coarse
    {
        int index[][2] = {{6, 6}, {6, 7}, {6, 8}, {6, 9}}, to_index[][2] = {{2, 3}, {2, 3}, {2, 4}, {2, 4}};
        HYPRE_SStructGraphAddEntries(graph, 1, index[0], c_var, 0, to_index[0], c_var);
        HYPRE_SStructGraphAddEntries(graph, 1, index[1], c_var, 0, to_index[1], c_var);
        HYPRE_SStructGraphAddEntries(graph, 1, index[2], c_var, 0, to_index[2], c_var);
        HYPRE_SStructGraphAddEntries(graph, 1, index[3], c_var, 0, to_index[3], c_var);
    }
    {
        int index[][2] = {{6, 6}, {7, 6}, {8, 6}, {9, 6}}, to_index[][2] = {{3, 2}, {3, 2}, {4, 2}, {4, 2}};
        HYPRE_SStructGraphAddEntries(graph, 1, index[0], c_var, 0, to_index[0], c_var);
        HYPRE_SStructGraphAddEntries(graph, 1, index[1], c_var, 0, to_index[1], c_var);
        HYPRE_SStructGraphAddEntries(graph, 1, index[2], c_var, 0, to_index[2], c_var);
        HYPRE_SStructGraphAddEntries(graph, 1, index[3], c_var, 0, to_index[3], c_var);
    }
    // coarse to fine
    {
        int to_index[][2] = {{6, 6}, {6, 7}, {6, 8}, {6, 9}}, index[][2] = {{2, 3}, {2, 3}, {2, 4}, {2, 4}};
        HYPRE_SStructGraphAddEntries(graph, 0, index[0], c_var, 1, to_index[0], c_var);
        HYPRE_SStructGraphAddEntries(graph, 0, index[1], c_var, 1, to_index[1], c_var);
        HYPRE_SStructGraphAddEntries(graph, 0, index[2], c_var, 1, to_index[2], c_var);
        HYPRE_SStructGraphAddEntries(graph, 0, index[3], c_var, 1, to_index[3], c_var);
    }
    {
        int to_index[][2] = {{6, 6}, {7, 6}, {8, 6}, {9, 6}}, index[][2] = {{3, 2}, {3, 2}, {4, 2}, {4, 2}};
        HYPRE_SStructGraphAddEntries(graph, 0, index[0], c_var, 1, to_index[0], c_var);
        HYPRE_SStructGraphAddEntries(graph, 0, index[1], c_var, 1, to_index[1], c_var);
        HYPRE_SStructGraphAddEntries(graph, 0, index[2], c_var, 1, to_index[2], c_var);
        HYPRE_SStructGraphAddEntries(graph, 0, index[3], c_var, 1, to_index[3], c_var);
    }
    HYPRE_SStructGraphAssemble(graph);

    // step 3: set up matrix & rhs
    HYPRE_SStructMatrix matrix;
    HYPRE_SStructVector x, b;
    HYPRE_SStructMatrixCreate(MPI_COMM_WORLD, graph, &matrix);
    HYPRE_SStructMatrixInitialize(matrix);
    // set the entries in matrix
    // part 0 - coarse grid
    for (int j = 1; j < 5; ++j) {
        for (int i = 1; i < 5; ++i) {
            int index[] = {i, j};
            int entries[] = {0, 1, 2, 3, 4};
            double vals[] = {4.0, -1.0, -1.0, -1.0, -1.0};
            HYPRE_SStructMatrixSetValues(matrix, 0, index, c_var, 5, entries, vals);
        }
    }
    // left physical bc
    for (int j = 1; j < 5; ++j) {
        int index[] = {1, j};
        int entries[] = {2};
        double vals[] = {0.};
        HYPRE_SStructMatrixSetValues(matrix, 0, index, c_var, 1, entries, vals);
    }
    // right physical bc
    for (int j = 1; j < 5; ++j) {
        int index[] = {4, j};
        int entries[] = {1};
        double vals[] = {0.};
        HYPRE_SStructMatrixSetValues(matrix, 0, index, c_var, 1, entries, vals);
    }
    // lower physical bc
    for (int i = 1; i < 5; ++i) {
        int index[] = {i, 1};
        int entries[] = {4};
        double vals[] = {0.};
        HYPRE_SStructMatrixSetValues(matrix, 0, index, c_var, 1, entries, vals);
    }
    // upper physical bc
    for (int i = 1; i < 5; ++i) {
        int index[] = {i, 4};
        int entries[] = {3};
        double vals[] = {0.};
        HYPRE_SStructMatrixSetValues(matrix, 0, index, c_var, 1, entries, vals);
    }
    // coarse-fine interface (2, 3) -> (2, 4)
    for (int j = 3; j < 5; ++j) {
        int index[] = {2, j};
        int entries[] = {0, 1, 5, 6};
        double vals[] = {3 + 4. / 3, 0., -2. / 3, -2. / 3};
        HYPRE_SStructMatrixSetValues(matrix, 0, index, c_var, 4, entries, vals);
    }
    // coarse-fine interface (3, 2) -> (4, 2)
    for (int i = 3; i < 5; ++i) {
        int index[] = {i, 2};
        int entries[] = {0, 3, 5, 6};
        double vals[] = {3 + 4. / 3., 0, -2. / 3., -2. / 3.};
        HYPRE_SStructMatrixSetValues(matrix, 0, index, c_var, 4, entries, vals);
    }
    // part 1 - fine grid
    for (int j = 6; j < 10; ++j) {
        for (int i = 6; i < 10; ++i) {
            int index[] = {i, j};
            int entries[] = {0, 1, 2, 3, 4};
            double vals[] = {16.0, -4., -4., -4., -4.};
            HYPRE_SStructMatrixSetValues(matrix, 1, index, c_var, 5, entries, vals);
        }
    }
    // right physical bc
    for (int j = 6; j < 10; ++j) {
        int index[] = {9, j};
        int entries[] = {1};
        double vals[] = {0.};
        HYPRE_SStructMatrixSetValues(matrix, 1, index, c_var, 1, entries, vals);
    }
    // upper physical bc
    for (int i = 6; i < 10; ++i) {
        int index[] = {i, 9};
        int entries[] = {3};
        double vals[] = {0.};
        HYPRE_SStructMatrixSetValues(matrix, 1, index, c_var, 1, entries, vals);
    }
    // fine-coarse interface (6, 6) -> (6, 9)
    for (int j = 6; j < 10; ++j) {
        int index[] = {6, j};
        int entries[] = {0};
        double vals[] = {-4. + 2. / 3. * 4.};
        HYPRE_SStructMatrixAddToValues(matrix, 1, index, c_var, 1, entries, vals);
        entries[0] = 2;
        vals[0] = 0.;
        HYPRE_SStructMatrixSetValues(matrix, 1, index, c_var, 1, entries, vals);
        entries[0] = 5;
        vals[0] = -2. / 3. * 4.;
        HYPRE_SStructMatrixSetValues(matrix, 1, index, c_var, 1, entries, vals);
    }
    // fine-coarse interface (6, 6) -> (9, 6)
    for (int i = 6; i < 10; ++i) {
        int index[] = {i, 6};
        int entries[] = {0};
        double vals[] = {-4. + 2. / 3. * 4.};
        HYPRE_SStructMatrixAddToValues(matrix, 1, index, c_var, 1, entries, vals);
        entries[0] = 4;
        vals[0] = 0.;
        HYPRE_SStructMatrixSetValues(matrix, 1, index, c_var, 1, entries, vals);
        entries[0] = (i == 6 ? 6 : 5);
        vals[0] = -2. / 3. * 4.;
        HYPRE_SStructMatrixSetValues(matrix, 1, index, c_var, 1, entries, vals);
    }
    int rfactors[][3] = {{1, 1, 1}, {2, 2, 1}};
    HYPRE_SStructFACZeroCFSten(matrix, grid, 1, rfactors[1]);
    HYPRE_SStructFACZeroFCSten(matrix, grid, 1);
    HYPRE_SStructFACZeroAMRMatrixData(matrix, 0, rfactors[1]);
    HYPRE_SStructMatrixAssemble(matrix);
    HYPRE_SStructMatrixPrint("A.mat", matrix, 0);

    HYPRE_SStructVectorCreate(MPI_COMM_WORLD, grid, &b);
    HYPRE_SStructVectorCreate(MPI_COMM_WORLD, grid, &x);
    HYPRE_SStructVectorInitialize(b);
    HYPRE_SStructVectorInitialize(x);
    for (int j = 1; j < 5; ++j) {
        for (int i = 1; i < 5; ++i) {
            int index[] = {i, j};
            double val = 1.;
            HYPRE_SStructVectorSetValues(b, 0, index, c_var, &val);
        }
    }
    for (int j = 6; j < 10; ++j) {
        for (int i = 6; i < 10; ++i) {
            int index[] = {i, j};
            double val = 1.;
            HYPRE_SStructVectorSetValues(b, 1, index, c_var, &val);
        }
    }
    int plevels[] = {0, 1};
    HYPRE_SStructFACZeroAMRVectorData(b, plevels, rfactors);
    HYPRE_SStructVectorAssemble(b);
    HYPRE_SStructVectorAssemble(x);
    HYPRE_SStructVectorPrint("b.vec", b, 0);

    HYPRE_SStructSolver solver;
    HYPRE_SStructFACCreate(MPI_COMM_WORLD, &solver);
    HYPRE_SStructFACSetMaxLevels(solver, 2);
    HYPRE_SStructFACSetMaxIter(solver, 20);
    HYPRE_SStructFACSetTol(solver, 1.0e-10);
    HYPRE_SStructFACSetPLevels(solver, 2, plevels);
    HYPRE_SStructFACSetPRefinements(solver, 2, rfactors);
    HYPRE_SStructFACSetRelChange(solver, 0);
    HYPRE_SStructFACSetCoarseSolverType(solver, 2);
    HYPRE_SStructFACSetLogging(solver, 1);
    HYPRE_SStructFACSetup2(solver, matrix, b, x);
    HYPRE_SStructFACSolve3(solver, matrix, b, x);
    int num_iterations;
    double final_res_norm;
    HYPRE_SStructFACGetNumIterations(solver, &num_iterations);
    HYPRE_SStructFACGetFinalRelativeResidualNorm(solver, &final_res_norm);
    HYPRE_SStructFACDestroy2(solver);
    std::cout << fmt::format("Iter = {}, Res = {}\n", num_iterations, final_res_norm);
}