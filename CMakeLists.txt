# ----------------------------------------------------------------------------
#
# Copyright (c) 2019 - 2021 by the OpFlow developers
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
#
# Modified from CMakeList.txt in spdlog (https://github.com/gabime/spdlog)
#
# ----------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.10)

# ----------------------------------------------------------------------------
# Start opflow project
# ----------------------------------------------------------------------------
include(cmake/utils.cmake)
include(cmake/ide.cmake)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/external/morse_cmake/modules")

opflow_extract_version()

project(opflow VERSION ${OPFLOW_VERSION} LANGUAGES C CXX)
message(STATUS "Build opflow: ${OPFLOW_VERSION}")

include(GNUInstallDirs)

# ----------------------------------------------------------------------------
# Set CMake policies to support later version behaviour
# ----------------------------------------------------------------------------
if (POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW) # option() honors variables already set
endif ()

# ----------------------------------------------------------------------------
# Set default build to release
# ----------------------------------------------------------------------------
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose Release or Debug" FORCE)
endif ()

# ----------------------------------------------------------------------------
# Compiler config
# ----------------------------------------------------------------------------
if (NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
elseif (CMAKE_CXX_STANDARD LESS 20)
    message(WARNING "OpFlow needs C++20 to compile. Changing the standard to C++20.")
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif ()
if (NOT CMAKE_C_STANDARD)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_C_STANDARD_REQUIRED ON)
elseif (CMAKE_C_STANDARD LESS 11)
    message(WARNING "OpFlow needs C11 to compile. Changing the standard to C11.")
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_C_STANDARD_REQUIRED ON)
endif ()

# make sure __cplusplus is defined when using msvc
if (MSVC)
    string(APPEND CMAKE_CXX_FLAGS " /Zc:__cplusplus /wd4267 /wd4390 /wd4244")
endif ()

set(CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_SYSTEM_NAME MATCHES "CYGWIN" OR CMAKE_SYSTEM_NAME MATCHES "MSYS")
    set(CMAKE_CXX_EXTENSIONS ON)
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_THREAD_LIBS_INIT "-lpthread")
    set(CMAKE_HAVE_THREADS_LIBRARY 1)
    set(CMAKE_USE_WIN32_THREADS_INIT 0)
    set(CMAKE_USE_PTHREADS_INIT 1)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")
endif ()

# ----------------------------------------------------------------------------
# Set OPFLOW_MASTER_PROJECT to ON if we are building opflow
# ----------------------------------------------------------------------------
# Check if opflow is being used directly or via add_subdirectory, but allow overriding
if (NOT DEFINED OPFLOW_MASTER_PROJECT)
    if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
        set(OPFLOW_MASTER_PROJECT ON)
    else ()
        set(OPFLOW_MASTER_PROJECT OFF)
    endif ()
endif ()

option(OPFLOW_BUILD_ALL "Build all artifacts" OFF)

# example options
option(OPFLOW_BUILD_EXAMPLES "Build OpFlow's example cases" OFF)

# document options
option(OPFLOW_BUILD_DOCS "Build OpFlow's document" OFF)

# datatype options
option(OPFLOW_SINGLE_PRECISION "Use float as default real type" OFF)

# performance options
option(OPFLOW_WITH_OPENMP "Build OpFlow with OpenMP support" ON)
option(OPFLOW_WITH_MPI "Build OpFlow with MPI support" ON)

# utility options
option(OPFLOW_WITH_HDF5 "Build OpFlow with HDF5 support" OFF)

# testing options
option(OPFLOW_BUILD_TESTS "Build tests" OFF)
option(OPFLOW_USE_BUNDLED_GTEST "Use bundled googletest" ON)

# benchmark options
option(OPFLOW_BUILD_BENCHMARKS "Build benchmarks" OFF)

# sanitizer options
option(OPFLOW_SANITIZE_ADDRESS "Enable address sanitizer in tests" OFF)
option(OPFLOW_SANITIZE_LEAK "Enable leak sanitizer in tests" OFF)
option(OPFLOW_SANITIZE_THREAD "Enable thread sanitizer in tests" OFF)
option(OPFLOW_SANITIZE_UB "Enable undefined behavior sanitizer in tests" OFF)

# warning options
option(OPFLOW_BUILD_WARNINGS "Enable compiler warnings" OFF)

# install options
option(OPFLOW_INSTALL "Generate the install target" ${OPFLOW_MASTER_PROJECT})
option(OPFLOW_FMT_EXTERNAL "Use external fmt library instead of bundled" OFF)
option(OPFLOW_FMT_EXTERNAL_HO "Use external fmt header-only library instead of bundled" OFF)
option(OPFLOW_SPDLOG_EXTERNAL "Use external spdlog library instead of bundled" OFF)
option(OPFLOW_SPDLOG_EXTERNAL_HO "Use external spdlog header-only library instead of bundled" OFF)
# the default behavior is to download & config HYPRE & VTK on-the-fly.
# set these options to avoid internet requirement
option(OPFLOW_WITH_VTK "Build OpFlow with VTK" ON)
option(OPFLOW_HYPRE_EXTERNAL "Use external HYPRE library instead of bundled" OFF)
option(OPFLOW_USE_BUNDLED_HYPRE "Use bundled HYPRE library" ON)
option(OPFLOW_VTK_EXTERNAL "Use external VTK library instead of bundled" OFF)
option(OPFLOW_VTK_PRE_DOWNLOAD "Use pre-downloaded VTK library" OFF)
option(OPFLOW_NO_EXCEPTIONS "Compile with -fno-exceptions. Call abort() on any OpFlow exceptions" OFF)

if (OPFLOW_FMT_EXTERNAL AND OPFLOW_FMT_EXTERNAL_HO)
    message(FATAL_ERROR "OPFLOW_FMT_EXTERNAL and OPFLOW_FMT_EXTERNAL_HO are mutually exclusive")
endif ()
if (OPFLOW_SPDLOG_EXTERNAL AND OPFLOW_SPDLOG_EXTERNAL_HO)
    message(FATAL_ERROR "OPFLOW_SPDLOG_EXTERNAL and OPFLOW_SPDLOG_EXTERNAL_HO are mutually exclusive")
endif ()
if (OPFLOW_HYPRE_EXTERNAL AND OPFLOW_HYPRE_PRE_DOWNLOAD)
    message(WARNING "OPFLOW_HYPRE_EXTERNAL and OPFLOW_HYPRE_PRE_DOWNLOAD are mutually exclusive. Use OPFLOW_HYPRE_EXTERNAL hereafter")
endif ()
if (OPFLOW_VTK_EXTERNAL AND OPFLOW_VTK_PRE_DOWNLAOD)
    message(WARNING "OPFLOW_VTK_EXTERNAL and OPFLOW_VTK_PRE_DOWNLOAD are mutually exclusive. Use OPFLOW_VTK_EXTERNAL hereafter")
endif ()

# clang-tidy
if (${CMAKE_VERSION} VERSION_GREATER "3.5")
    option(OPFLOW_TIDY "run clang-tidy" OFF)
endif ()

if (OPFLOW_TIDY)
    set(CMAKE_CXX_CLANG_TIDY "clang-tidy")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    message(STATUS "Enabled clang-tidy")
endif ()

message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})

# ----------------------------------------------------------------------------
# Header only version
# ----------------------------------------------------------------------------
add_library(opflow INTERFACE)
add_library(opflow::opflow ALIAS opflow)

target_include_directories(opflow INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/src>")

# ----------------------------------------------------------------------------
# Use fmt package
# ----------------------------------------------------------------------------
if (OPFLOW_FMT_EXTERNAL OR OPFLOW_FMT_EXTERNAL_HO)
    if (NOT TARGET fmt::fmt)
        find_package(fmt CONFIG REQUIRED)
    endif ()
    target_compile_definitions(opflow INTERFACE OPFLOW_FMT_EXTERNAL)

    # use external fmt-header-only
    if (OPFLOW_FMT_EXTERNAL_HO)
        target_link_libraries(opflow INTERFACE fmt::fmt-header-only)
    else () # use external compile fmt
        target_link_libraries(opflow INTERFACE fmt::fmt)
    endif ()

    set(PKG_CONFIG_REQUIRES fmt) # add dependency to pkg-config
else ()
    # use the header-only fmt by default
    target_include_directories(opflow INTERFACE
            "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/external/fmt/include>"
            "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
endif ()

# ----------------------------------------------------------------------------
# Use spdlog package
# ----------------------------------------------------------------------------
if (OPFLOW_SPDLOG_EXTERNAL OR OPFLOW_SPDLOG_EXTERNAL_HO)
    if (NOT TARGET spdlog::spdlog)
        find_package(spdlog CONFIG REQUIRED)
    endif ()
    target_compile_definitions(opflow INTERFACE OPFLOW_SPDLOG_EXTERNAL)

    # use external spdlog-header-only
    if (OPFLOW_SPDLOG_EXTERNAL_HO)
        target_link_libraries(opflow INTERFACE spdlog::spdlog_header_only)
    else () # use external compile spdlog
        target_link_libraries(opflow INTERFACE spdlog::spdlog)
    endif ()

    list(APPEND PKG_CONFIG_REQUIRES spdlog) # add dependency to pkg-config
else ()
    # use the header-only spdlog by default
    target_include_directories(opflow INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/external/spdlog/include>"
            "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
endif ()

# ----------------------------------------------------------------------------
# Use AMGCL
# ----------------------------------------------------------------------------
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/external/amgcl EXCLUDE_FROM_ALL)
target_link_libraries(opflow INTERFACE $<BUILD_INTERFACE:amgcl::amgcl>)

# ----------------------------------------------------------------------------
# Find MPI
# ----------------------------------------------------------------------------
if (OPFLOW_WITH_MPI)
    find_package(MPI REQUIRED)
    target_link_libraries(opflow INTERFACE MPI::MPI_C MPI::MPI_CXX)

    list(APPEND PKG_CONFIG_REQUIRES MPI)
endif ()

# ----------------------------------------------------------------------------
# Find OpenMP
# ----------------------------------------------------------------------------
if (OPFLOW_WITH_OPENMP)
    find_package(OpenMP REQUIRED)
    target_link_libraries(opflow INTERFACE OpenMP::OpenMP_C OpenMP::OpenMP_CXX)
    target_compile_options(opflow INTERFACE ${OpenMP_CXX_FLAGS})

    list(APPEND PKG_CONFIG_REQUIRES OpenMP)
endif ()

# ----------------------------------------------------------------------------
# Use HYPRE package
# ----------------------------------------------------------------------------
if (OPFLOW_HYPRE_EXTERNAL)
    if (NOT TARGET HYPRE)
        if (OPFLOW_WITH_OPENMP)
            set(CMAKE_REQUIRED_FLAGS "-fopenmp")
        endif ()
        find_package(HYPRE REQUIRED)
        target_compile_definitions(opflow INTERFACE OPFLOW_HYPRE_EXTERNAL)
    endif ()
else ()
    include(cmake/HYPREConfig.cmake)
    CONFIG_HYPRE()
endif ()
list(APPEND PKG_CONFIG_REQUIRES HYPRE)
target_link_libraries(opflow INTERFACE HYPRE::HYPRE)

# ----------------------------------------------------------------------------
# Use VTK package
# ----------------------------------------------------------------------------
if (OPFLOW_WITH_VTK)
    target_compile_definitions(opflow INTERFACE OPFLOW_WITH_VTK)
    if (OPFLOW_VTK_EXTERNAL)
        if (NOT TARGET VTK)
            find_package(VTK REQUIRED)
            target_compile_definitions(opflow INTERFACE OPFLOW_VTK_EXTERNAL)
        endif ()
    else ()
        include(cmake/VTKConfig.cmake)
        CONFIG_VTK()
    endif ()
    list(APPEND PKG_CONFIG_REQUIRES VTK)
    target_include_directories(opflow INTERFACE ${VTK_INCLUDE_DIRS})
    target_link_directories(opflow INTERFACE ${VTK_LIBRARY_DIRS})
    target_link_libraries(opflow INTERFACE ${VTK_LIBRARIES})
endif ()

# ----------------------------------------------------------------------------
# Use HDF5 package
# ----------------------------------------------------------------------------
if (OPFLOW_WITH_HDF5)
    if (OPFLOW_WITH_MPI)
        set(HDF5_PREFER_PARALLEL TRUE)
    endif ()
    set(HDF5_USE_STATIC_LIBRARIES ON)
    find_package(HDF5 COMPONENTS C REQUIRED)
    if (NOT HDF5_IS_PARALLEL)
        message(FATAL_ERROR "Cannot find parallel HDF5 library")
    endif ()
    target_include_directories(opflow INTERFACE ${HDF5_INCLUDE_DIRS})
    target_link_libraries(opflow INTERFACE ${HDF5_LIBRARIES})
    target_compile_definitions(opflow INTERFACE OPFLOW_WITH_HDF5)
    if (OPFLOW_WITH_MPI)
        target_compile_definitions(opflow INTERFACE H5_HAVE_PARALLEL)
    endif ()
endif ()

# ----------------------------------------------------------------------------
# Platform detection
# ----------------------------------------------------------------------------
if (UNIX)
    target_compile_definitions(opflow INTERFACE OPFLOW_PLATFORM_UNIX)
elseif (APPLE)
    target_compile_definitions(opflow INTERFACE OPFLOW_PLATFORM_APPLE)
elseif (WIN32)
    target_compile_definitions(opflow INTERFACE OPFLOW_PLATFORM_WIN)
endif ()

# ----------------------------------------------------------------------------
# Language & library features detection
# ----------------------------------------------------------------------------
include(CheckIncludeFiles)
check_include_files("sys/mman.h" HAS_SYS_MMAN_H)
if (HAS_SYS_MMAN_H)
    target_compile_definitions(opflow INTERFACE OPFLOW_HAS_MMAN_H)
endif ()

# ----------------------------------------------------------------------------
# Misc definitions according to tweak options
# ----------------------------------------------------------------------------
foreach (
        OPFLOW_OPTION
        OPFLOW_SINGLE_PRECISION
        OPFLOW_WITH_MPI
        OPFLOW_WITH_OPENMP)
    if (${OPFLOW_OPTION})
        target_compile_definitions(opflow INTERFACE ${OPFLOW_OPTION})
    endif ()
endforeach ()

if (OPFLOW_NO_EXCEPTIONS AND NOT MSVC)
    target_compile_options(opflow INTERFACE -fno-exceptions)
endif ()

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
# Build binaries
# ----------------------------------------------------------------------------
if (OPFLOW_BUILD_EXAMPLES OR OPFLOW_BUILD_ALL)
    message(STATUS "Generating examples")
    add_subdirectory(examples)
endif ()

# ----------------------------------------------------------------------------
# Build tests
# ----------------------------------------------------------------------------
if (OPFLOW_BUILD_TESTS OR OPFLOW_BUILD_ALL)
    message(STATUS "Generating tests")
    enable_testing()
    add_subdirectory(test)
endif ()

# ----------------------------------------------------------------------------
# Build benchmarks
# ----------------------------------------------------------------------------
if (OPFLOW_BUILD_BENCHMARKS OR OPFLOW_BUILD_ALL)
    message(STATUS "Generating benchmarks")
    # config google benchmark as part of the project
    set(BENCHMARK_ENABLE_INSTALL OFF)
    set(BENCHMARK_ENABLE_TESTING OFF)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/external/benchmark EXCLUDE_FROM_ALL)
    add_subdirectory(benchmark)
endif ()

# ----------------------------------------------------------------------------
# Build docs
# ----------------------------------------------------------------------------
if (OPFLOW_BUILD_DOCS OR OPFLOW_BUILD_ALL)
    message(STATUS "Generating docs")
    find_program(SPHINX_PATH sphinx-build
            HINTS $ENV{PATH})
    if (NOT SPHINX_PATH)
        message(FATAL_ERROR "sphinx-build not found. Please install it via \
        instructions at https://www.sphinx-doc.org/en/master/usage/installation.html")
    else ()
        message(STATUS "Found sphinx-build at ${SPHINX_PATH}")
    endif ()
    find_program(DOXYGEN_PATH doxygen
            HINTS ${DOXYGEN_DIR}
            REQUIRED
            )
    if (NOT DOXYGEN_PATH)
        message(FATAL_ERROR "Doxygen not found. Please install doxygen with version >= 1.9.2")
    else ()
        execute_process(
                COMMAND ${DOXYGEN_PATH} --version
                RESULT_VARIABLE _
                OUTPUT_VARIABLE _out
        )
        string(REGEX REPLACE "\n$" "" _out "${_out}")
        if (${_out} VERSION_LESS "1.9.2")
            message(FATAL_ERROR "Doxygen version too low. Found v${_out}, require >= v1.9.2")
        endif ()
    endif ()
    message(STATUS "Found Doxygen version: ${_out}")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    add_custom_target(doxygen_doc
            COMMAND ${DOXYGEN_PATH} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    add_custom_target(doc
            COMMAND ${SPHINX_PATH} -b html
            -Dbreathe_projects.OpFlow=${CMAKE_CURRENT_BINARY_DIR}/doc/doxygen/xml
            ${CMAKE_CURRENT_LIST_DIR}/doc ${CMAKE_CURRENT_BINARY_DIR}/doc
            DEPENDS doxygen_doc
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endif ()

# ----------------------------------------------------------------------------
# Install
# ----------------------------------------------------------------------------
if (OPFLOW_INSTALL)
    message(STATUS "Generating install")
    IF (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        SET(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}" CACHE PATH "" FORCE)
        message(STATUS "Install prefix set to: ${CMAKE_INSTALL_PREFIX}")
    ENDIF (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
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
    install(
            TARGETS opflow
            EXPORT opflow
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    if (NOT OPFLOW_FMT_EXTERNAL AND NOT OPFLOW_FMT_EXTERNAL_HO)
        install(DIRECTORY external/fmt/include/fmt/
                DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/fmt")
    endif ()
    if (NOT OPFLOW_SPDLOG_EXTERNAL AND NOT OPFLOW_SPDLOG_EXTERNAL_HO)
        install(DIRECTORY external/spdlog/include/spdlog/
                DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/spdlog")
    endif ()
    if (NOT OPFLOW_HYPRE_EXTERNAL)
        install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/hypre/
                DESTINATION "${CMAKE_INSTALL_PREFIX}")
    endif ()
    if (OPFLOW_WITH_VTK AND NOT OPFLOW_VTK_EXTERNAL)
        install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/vtk/
                DESTINATION "${CMAKE_INSTALL_PREFIX}")
    endif ()
    # Build & install AMGCL
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/AMGCLConfig.cmake.in
            ${CMAKE_CURRENT_BINARY_DIR}/cmake/AMGCLConfig.cmake)
    install(CODE "
    include(${CMAKE_CURRENT_BINARY_DIR}/cmake/AMGCLConfig.cmake)
    CONFIG_AND_INSTALL_AMGCL()")

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
endif ()

# uninstall target
if (NOT TARGET uninstall)
    configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
            "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
            IMMEDIATE @ONLY)

    add_custom_target(uninstall
            COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif ()
