cmake_minimum_required(VERSION 3.10)
project(OpFlow_Cases_FTCS-MPI)

if (NOT TARGET opflow)
    find_package(opflow REQUIRED)
endif ()

add_executable(FTCS-OMP EXCLUDE_FROM_ALL FTCS-OMP.cpp)
target_link_libraries(FTCS-OMP opflow::opflow)
target_compile_options(FTCS-OMP PRIVATE ${OPFLOW_EXAMPLE_FLAGS})
target_compile_definitions(FTCS-OMP PRIVATE OPFLOW_THREAD_MODEL_OPENMP)


add_executable(FTCS-MPI EXCLUDE_FROM_ALL FTCS-MPI.cpp)
target_link_libraries(FTCS-MPI opflow::opflow)
target_compile_options(FTCS-MPI PRIVATE ${OPFLOW_EXAMPLE_FLAGS})
target_compile_definitions(FTCS-MPI PRIVATE OPFLOW_DISTRIBUTE_MODEL_MPI)

add_executable(FTCS-MPI-OMP EXCLUDE_FROM_ALL FTCS-MPI-OMP.cpp)
target_link_libraries(FTCS-MPI-OMP opflow::opflow)
target_compile_options(FTCS-MPI-OMP PRIVATE ${OPFLOW_EXAMPLE_FLAGS})
target_compile_definitions(FTCS-MPI-OMP PRIVATE OPFLOW_DISTRIBUTE_MODEL_MPI
        PRIVATE OPFLOW_THREAD_MODEL_OPENMP)