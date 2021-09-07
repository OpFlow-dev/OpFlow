#include "GridInit.hpp"
#include "GridUpdate.hpp"
#include "Poisson.hpp"
#include "pch.hpp"
int hypre_test(int argc, char* argv[]);
int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    //amrls();
    Poisson();
    MPI_Finalize();
    //hypre_test(argc, argv);
}