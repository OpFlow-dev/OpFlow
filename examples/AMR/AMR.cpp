#include "GridInit.hpp"
#include "GridUpdate.hpp"
#include "Poisson.hpp"
#include "pch.hpp"
int hypre_test(int argc, char* argv[]);
int main(int argc, char** argv) {
    OpFlow::EnvironmentGardian _(&argc, &argv);
    //amrls();
    Poisson();
    //hypre_test(argc, argv);
}