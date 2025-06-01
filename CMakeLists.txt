# ----------------------------------------------------------------------------
#
# Copyright (c) 2019 - 2023 by the OpFlow developers
#
# This file is part of OpFlow.
#
# OpFlow is free software and is distributed under the MPL v2.0 license.
# The full text of the license can be found in the file LICENSE at the top
# level directory of OpFlow.
#
# ----------------------------------------------------------------------------
#
# The main CMakeList for OpFlow
#
# ----------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.13)

# ----------------------------------------------------------------------------
# Start opflow project
# ----------------------------------------------------------------------------
include(cmake/utils.cmake)
include(cmake/ide.cmake)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
string(SUBSTRING ${GIT_SHA1} 0 8 SHORT_SHA1)
git_local_changes(GIT_CHANGE)

opflow_extract_version()

project(opflow VERSION ${OPFLOW_VERSION} LANGUAGES C CXX)
set(OPFLOW_VERSION_STRING "${OPFLOW_VERSION} - ${SHORT_SHA1} - ${GIT_CHANGE}")
message(STATUS "Build opflow: ${OPFLOW_VERSION_STRING}")

include(GNUInstallDirs)

# ----------------------------------------------------------------------------
# Set CMake policies to support later version behaviour
# ----------------------------------------------------------------------------
if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW) # option() honors variables already set
endif()

# ----------------------------------------------------------------------------
# Set default build to release
# ----------------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose Release or Debug" FORCE)
endif()

# ----------------------------------------------------------------------------
# Compiler config
# ----------------------------------------------------------------------------
if(NOT CMAKE_C_STANDARD)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_C_STANDARD_REQUIRED ON)
elseif(CMAKE_C_STANDARD LESS 11)
    message(WARNING "OpFlow needs C11 to compile. Changing the standard to C11.")
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_C_STANDARD_REQUIRED ON)
endif()

# make sure __cplusplus is defined when using msvc
if(MSVC)
    string(APPEND CMAKE_CXX_FLAGS " /Zc:__cplusplus /wd4267 /wd4390 /wd4244")
endif()

# supress possible abi warning by upgrade compiler
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi")
# temporary fix for macOS 14 & CLT 15
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin"
    AND CMAKE_HOST_SYSTEM_VERSION VERSION_GREATER_EQUAL 23)
    add_link_options(-Wl,-ld_classic)
endif()

set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_SYSTEM_NAME MATCHES "CYGWIN" OR CMAKE_SYSTEM_NAME MATCHES "MSYS")
    set(CMAKE_CXX_EXTENSIONS ON)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_THREAD_LIBS_INIT "-lpthread")
    set(CMAKE_HAVE_THREADS_LIBRARY 1)
    set(CMAKE_USE_WIN32_THREADS_INIT 0)
    set(CMAKE_USE_PTHREADS_INIT 1)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    #set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")
endif()

# ----------------------------------------------------------------------------
# Set OPFLOW_MASTER_PROJECT to ON if we are building opflow
# ----------------------------------------------------------------------------
# Check if opflow is being used directly or via add_subdirectory, but allow overriding
if(NOT DEFINED OPFLOW_MASTER_PROJECT)
    if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
        set(OPFLOW_MASTER_PROJECT ON)
    else()
        set(OPFLOW_MASTER_PROJECT OFF)
    endif()
endif()

option(OPFLOW_BUILD_ALL "Build all artifacts" OFF)

# example options
option(OPFLOW_BUILD_EXAMPLES "Build OpFlow's example cases" OFF)

# document options
option(OPFLOW_BUILD_DOCS "Build OpFlow's document" OFF)

# datatype options
option(OPFLOW_SINGLE_PRECISION "Use float as default real type" OFF)

# performance options
option(OPFLOW_WITH_OPENMP "Build OpFlow with OpenMP support" ON)
option(OPFLOW_WITH_MPI "Build OpFlow with MPI support" OFF)

# utility options
option(OPFLOW_WITH_HDF5 "Build OpFlow with HDF5 support" OFF)

# testing options
option(OPFLOW_BUILD_TESTS "Build tests" OFF)
option(OPFLOW_USE_BUNDLED_GTEST "Use bundled googletest" ON)
option(OPFLOW_ENABLE_COVERAGE "Enable coverage" OFF)

# benchmark options
option(OPFLOW_BUILD_BENCHMARKS "Build benchmarks" OFF)

# sanitizer options
option(OPFLOW_SANITIZE_ADDRESS "Enable address sanitizer in tests" OFF)
option(OPFLOW_SANITIZE_LEAK "Enable leak sanitizer in tests" OFF)
option(OPFLOW_SANITIZE_THREAD "Enable thread sanitizer in tests" OFF)
option(OPFLOW_SANITIZE_UB "Enable undefined behavior sanitizer in tests" OFF)

# compile options
option(OPFLOW_BUILD_WARNINGS "Enable compiler warnings" OFF)
option(OPFLOW_NO_EXCEPTIONS "Compile with -fno-exceptions" OFF)

# install options
option(OPFLOW_INSTALL "Generate the install target" ${OPFLOW_MASTER_PROJECT})
# the default behavior is to use bundled HYPRE, VTK & TBB
option(OPFLOW_WITH_VTK "Build OpFlow with VTK" OFF)
option(OPFLOW_HYPRE_EXTERNAL "Use external HYPRE library instead of bundled" OFF)
option(OPFLOW_VTK_EXTERNAL "Use external VTK library instead of bundled" OFF)
option(OPFLOW_TBB_EXTERNAL "Use external Intel TBB library instead of bundled" OFF)

# clang-tidy
if(${CMAKE_VERSION} VERSION_GREATER "3.5")
    option(OPFLOW_TIDY "run clang-tidy" OFF)
endif()

if(OPFLOW_TIDY)
    set(CMAKE_CXX_CLANG_TIDY "clang-tidy")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    message(STATUS "Enabled clang-tidy")
endif()

# ----------------------------------------------------------------------------
# Compile options
# ----------------------------------------------------------------------------
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-std=c++26 c++26support)
if(NOT c++26support)
    message(FATAL_ERROR "OpFlow requires C++26 to compile.")
endif()

message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})
add_subdirectory(src)

# ----------------------------------------------------------------------------
# External dependencies
# ----------------------------------------------------------------------------
add_subdirectory(external)

# ----------------------------------------------------------------------------
# Find MPI
# ----------------------------------------------------------------------------
if(OPFLOW_WITH_MPI)
    find_package(MPI REQUIRED)
    target_link_libraries(opflow INTERFACE MPI::MPI_C MPI::MPI_CXX)

    list(APPEND PKG_CONFIG_REQUIRES MPI)
endif()

# ----------------------------------------------------------------------------
# Find OpenMP
# ----------------------------------------------------------------------------
if(OPFLOW_WITH_OPENMP)
    # workarounds for CI to find the correct libomp
    if(APPLE)
        if(EXISTS /usr/local/opt/llvm/lib)
            link_directories("/usr/local/opt/llvm/lib")
        elseif(EXISTS /opt/homebrew/lib)
            link_directories("/opt/homebrew/lib")
        endif()
    endif()
    if(CMAKE_C_COMPILER_ID MATCHES "Clang")
        set(OpenMP_C "${CMAKE_C_COMPILER}")
        set(OpenMP_C_FLAGS "-fopenmp=libomp -Wno-unused-command-line-argument -Wno-deprecated-pragma")
        set(OpenMP_C_LIB_NAMES "omp")
        set(OpenMP_omp_LIBRARY ${OpenMP_C_LIB_NAMES})
    endif()
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(OpenMP_CXX "${CMAKE_CXX_COMPILER}")
        set(OpenMP_CXX_FLAGS "-fopenmp=libomp -Wno-unused-command-line-argument")
        set(OpenMP_CXX_LIB_NAMES "omp")
        set(OpenMP_omp_LIBRARY ${OpenMP_CXX_LIB_NAMES})
    endif()
    find_package(OpenMP REQUIRED)
    target_link_libraries(opflow INTERFACE OpenMP::OpenMP_C OpenMP::OpenMP_CXX)
    target_compile_options(opflow INTERFACE ${OpenMP_CXX_FLAGS})

    list(APPEND PKG_CONFIG_REQUIRES OpenMP)
endif()

# ----------------------------------------------------------------------------
# Platform detection
# ----------------------------------------------------------------------------
if(UNIX)
    target_compile_definitions(opflow INTERFACE OPFLOW_PLATFORM_UNIX)
elseif(APPLE)
    target_compile_definitions(opflow INTERFACE OPFLOW_PLATFORM_APPLE)
elseif(WIN32)
    target_compile_definitions(opflow INTERFACE OPFLOW_PLATFORM_WIN)
endif()

# ----------------------------------------------------------------------------
# Language & library features detection
# ----------------------------------------------------------------------------
include(CheckIncludeFiles)
check_include_files("sys/mman.h" HAS_SYS_MMAN_H)
if(HAS_SYS_MMAN_H)
    target_compile_definitions(opflow INTERFACE OPFLOW_HAS_MMAN_H)
endif()

# ----------------------------------------------------------------------------
# Misc definitions according to tweak options
# ----------------------------------------------------------------------------
foreach(
    OPFLOW_OPTION
    OPFLOW_SINGLE_PRECISION
    OPFLOW_WITH_MPI
    OPFLOW_WITH_OPENMP)
    if(${OPFLOW_OPTION})
        target_compile_definitions(opflow INTERFACE ${OPFLOW_OPTION})
    endif()
endforeach()
if(OPFLOW_WITH_MPI)
    target_compile_definitions(opflow INTERFACE OPFLOW_DISTRIBUTE_MODEL_MPI)
endif()

if(OPFLOW_NO_EXCEPTIONS AND NOT MSVC)
    target_compile_options(opflow INTERFACE -fno-exceptions)
endif()

# sanitizer flags
target_compile_options(opflow
    INTERFACE $<$<BOOL:${OPFLOW_SANITIZE_ADDRESS}>: -fsanitize=address>
    INTERFACE $<$<BOOL:${OPFLOW_SANITIZE_LEAK}>: -fsanitize=leak>
    INTERFACE $<$<BOOL:${OPFLOW_SANITIZE_THREAD}>: -fsanitize=thread>
    INTERFACE $<$<BOOL:${OPFLOW_SANITIZE_UB}>: -fsanitize=undefined>)
target_link_options(opflow
    INTERFACE $<$<BOOL:${OPFLOW_SANITIZE_ADDRESS}>: -fsanitize=address>
    INTERFACE $<$<BOOL:${OPFLOW_SANITIZE_LEAK}>: -fsanitize=leak>
    INTERFACE $<$<BOOL:${OPFLOW_SANITIZE_THREAD}>: -fsanitize=thread>
    INTERFACE $<$<BOOL:${OPFLOW_SANITIZE_UB}>: -fsanitize=undefined>)

# ----------------------------------------------------------------------------
# All_CI target (= AllExamples + AllBenchmarks + AllTests)
# ----------------------------------------------------------------------------
add_custom_target(All_CI)

# ----------------------------------------------------------------------------
# Build binaries
# ----------------------------------------------------------------------------
if(OPFLOW_BUILD_EXAMPLES OR OPFLOW_BUILD_ALL)
    message(STATUS "Generating examples")
    add_subdirectory(examples)
endif()

# ----------------------------------------------------------------------------
# Build tests
# ----------------------------------------------------------------------------
if(OPFLOW_BUILD_TESTS OR OPFLOW_BUILD_ALL)
    message(STATUS "Generating tests")
    enable_testing()
    add_subdirectory(test)
endif()

# ----------------------------------------------------------------------------
# Build benchmarks
# ----------------------------------------------------------------------------
if(OPFLOW_BUILD_BENCHMARKS OR OPFLOW_BUILD_ALL)
    message(STATUS "Generating benchmarks")
    # config google benchmark as part of the project
    set(BENCHMARK_ENABLE_INSTALL OFF)
    set(BENCHMARK_ENABLE_TESTING OFF)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/external/benchmark EXCLUDE_FROM_ALL)
    add_subdirectory(benchmark)
endif()

# ----------------------------------------------------------------------------
# Build docs
# ----------------------------------------------------------------------------
if(OPFLOW_BUILD_DOCS OR OPFLOW_BUILD_ALL)
    message(STATUS "Generating docs")
    find_program(SPHINX_PATH sphinx-build
        HINTS $ENV{PATH} ${SPHINX_DIR})
    if(NOT SPHINX_PATH)
        message(FATAL_ERROR "sphinx-build not found. Please install it via \
        instructions at https://www.sphinx-doc.org/en/master/usage/installation.html")
    else()
        message(STATUS "Found sphinx-build at ${SPHINX_PATH}")
    endif()
    find_program(DOXYGEN_PATH doxygen
        HINTS ${DOXYGEN_DIR}
        REQUIRED
    )
    if(NOT DOXYGEN_PATH)
        message(FATAL_ERROR "Doxygen not found. Please install doxygen with version >= 1.9.2")
    else()
        execute_process(
            COMMAND ${DOXYGEN_PATH} --version
            RESULT_VARIABLE _
            OUTPUT_VARIABLE _out
        )
        string(REGEX REPLACE "\n$" "" _out "${_out}")
        if(${_out} VERSION_LESS "1.9.2")
            message(FATAL_ERROR "Doxygen version too low. Found v${_out}, require >= v1.9.2")
        endif()
    endif()
    message(STATUS "Found Doxygen version: ${_out}")
    configure_file(doc/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doc/doxygen)
    add_custom_target(doxygen_doc
        COMMAND ${DOXYGEN_PATH} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    add_custom_target(doc
        COMMAND ${SPHINX_PATH} -b html
        -Dbreathe_projects.OpFlow=${CMAKE_CURRENT_BINARY_DIR}/doc/doxygen/xml
        ${CMAKE_CURRENT_LIST_DIR}/doc ${CMAKE_CURRENT_BINARY_DIR}/doc
        DEPENDS doxygen_doc
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endif()

# ----------------------------------------------------------------------------
# Install
# ----------------------------------------------------------------------------
if(OPFLOW_INSTALL)
    message(STATUS "Generating install")
    IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        SET(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}" CACHE PATH "" FORCE)
        message(STATUS "Install prefix set to: ${CMAKE_INSTALL_PREFIX}")
    ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(project_config_in "${CMAKE_CURRENT_LIST_DIR}/cmake/opflowConfig.cmake.in")
    set(project_config_out "${CMAKE_CURRENT_BINARY_DIR}/opflowConfig.cmake")
    set(config_targets_file "opflowConfigTargets.cmake")
    set(version_config_file "${CMAKE_CURRENT_BINARY_DIR}/opflowConfigVersion.cmake")
    set(export_dest_dir "${CMAKE_INSTALL_LIBDIR}/cmake/opflow")
    set(pkgconfig_install_dir "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
    set(pkg_config "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc")

    # ----------------------------------------------------------------------------
    # Include files
    # ----------------------------------------------------------------------------
    install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
    install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/src/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/src")
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Version.hpp DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
    install(
        TARGETS opflow
        EXPORT opflow
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    if(OPFLOW_WITH_VTK AND NOT OPFLOW_VTK_EXTERNAL)
        install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/vtk/
            DESTINATION "${CMAKE_INSTALL_PREFIX}")
    endif()

    # ----------------------------------------------------------------------------
    # Install CMake config files
    # ----------------------------------------------------------------------------
    install(EXPORT opflow DESTINATION ${export_dest_dir} NAMESPACE opflow:: FILE ${config_targets_file})

    include(CMakePackageConfigHelpers)
    configure_file("${project_config_in}" "${project_config_out}" @ONLY)

    write_basic_package_version_file("${version_config_file}" COMPATIBILITY SameMajorVersion)
    install(FILES "${project_config_out}" "${version_config_file}" DESTINATION "${export_dest_dir}")

    # ----------------------------------------------------------------------------
    # Support creation of installable packages
    # ----------------------------------------------------------------------------
    #include(cmake/opflowCPack.cmake)
endif()

# uninstall target
if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY)

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()
