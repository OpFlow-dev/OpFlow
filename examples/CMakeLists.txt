cmake_minimum_required(VERSION 3.10)
project(OpFlow_Examples)

# set cpp flags for opflow's examples
set(OPFLOW_EXAMPLE_CXX_FLAGS)
set(OPFLOW_EXAMPLE_CXX_FLAGS_DEBUG)
set(OPFLOW_EXAMPLE_CXX_FLAGS_RELEASE)
set(OPFLOW_EXAMPLE_CXX_FLAGS_RELWITHDEBINFO)
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    string(APPEND OPFLOW_EXAMPLE_CXX_FLAGS "-mprefer-vector-width=512 -mfma \
             -Wno-narrowing -ftemplate-backtrace-limit=0")
    string(APPEND OPFLOW_EXAMPLE_CXX_FLAGS_DEBUG "${OPFLOW_EXAMPLE_CXX_FLAGS} -g -O0")
    string(APPEND OPFLOW_EXAMPLE_CXX_FLAGS_RELEASE "${OPFLOW_EXAMPLE_CXX_FLAGS} -O3 -fopenmp-simd -DNDEBUG")
    string(APPEND OPFLOW_EXAMPLE_CXX_FLAGS_RELWITHDEBINFO "${OPFLOW_EXAMPLE_CXX_FLAGS_RELEASE} -g")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    message(FATAL_ERROR "Intel compiler doesn't support C++20 standard.")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    string(APPEND OPFLOW_EXAMPLE_CXX_FLAGS_DEBUG "/Od")
    string(APPEND OPFLOW_EXAMPLE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG")
    string(APPEND OPFLOW_EXAMPLE_CXX_FLAGS_RELWITHDEBINFO "${OPFLOW_EXAMPLE_CXX_FLAGS_RELEASE} /Od")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    string(APPEND OPFLOW_EXAMPLE_CXX_FLAGS "-mfma -Wno-narrowing -Wno-deprecated-anon-enum-enum-conversion")
    string(APPEND OPFLOW_EXAMPLE_CXX_FLAGS_DEBUG "${OPFLOW_EXAMPLE_CXX_FLAGS} -O0 -g")
    string(APPEND OPFLOW_EXAMPLE_CXX_FLAGS_RELEASE "${OPFLOW_EXAMPLE_CXX_FLAGS} -O3 -fopenmp-simd -DNDEBUG")
endif ()

set(OPFLOW_EXAMPLE_FLAGS "$<$<CONFIG:Debug>:${OPFLOW_EXAMPLE_CXX_FLAGS_DEBUG}> \
    $<$<CONFIG:Release>:${OPFLOW_EXAMPLE_CXX_FLAGS_RELEASE}> \
    $<$<CONFIG:RelWithDebInfo>:${OPFLOW_EXAMPLE_CXX_FLAGS_RELWITHDEBINFO}>")

string(REPLACE " " ";" OPFLOW_EXAMPLE_FLAGS ${OPFLOW_EXAMPLE_FLAGS})

add_custom_target(AllExamples)

function(add_example example_name)
    add_executable(${example_name} EXCLUDE_FROM_ALL ${ARGN})
    target_link_libraries(${example_name} opflow::opflow)
    target_compile_options(${example_name} PRIVATE ${OPFLOW_EXAMPLE_FLAGS})
    add_dependencies(AllExamples ${example_name})
endfunction()

function(add_mpi_example example_name)
    if (OPFLOW_WITH_MPI)
        add_example(${example_name} ${ARGN})
    endif ()
endfunction()

add_subdirectory(LidDriven)
add_subdirectory(FTCS)
add_subdirectory(CONV1D)
add_subdirectory(EqnHandler)
add_subdirectory(FTCS2D)
add_subdirectory(TaylorGreen)
# AMR Cases
add_subdirectory(AMR)
add_subdirectory(LevelSet)

add_dependencies(All_CI AllExamples)