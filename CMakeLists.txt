# ----------------------------------------------------------------------------
#
# Copyright (c) 2019 - 2026 by the OpFlow developers
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

cmake_minimum_required(VERSION 4.0.2 FATAL_ERROR)

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
# IO is always enabled (no variant dimension).
set(OPFLOW_WITH_HDF5 ON CACHE BOOL "Build OpFlow with HDF5 support (always ON)" FORCE)

# testing options
option(OPFLOW_BUILD_TESTS "Build tests" OFF)
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
option(OPFLOW_ENABLE_MODULE "Enable C++ modules support" OFF)
if(OPFLOW_ENABLE_MODULE)
    set(CMAKE_CXX_MODULE_STD 1)
    set(OPFLOW_TARGET_SCOPE PUBLIC)
else()
    # Disable global module dependency scanning for non-module builds.
    set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
    set(OPFLOW_TARGET_SCOPE INTERFACE)
endif()

# install options
option(OPFLOW_INSTALL "Generate the install target" ${PROJECT_IS_TOP_LEVEL})
# VTK is always enabled (no variant dimension).
set(OPFLOW_WITH_VTK ON CACHE BOOL "Build OpFlow with VTK support (always ON)" FORCE)

# clang-tidy
if(${CMAKE_VERSION} VERSION_GREATER "3.5")
    option(OPFLOW_TIDY "run clang-tidy" OFF)
endif()

if(OPFLOW_TIDY)
    set(CMAKE_CXX_CLANG_TIDY "clang-tidy")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    message(STATUS "Enabled clang-tidy")
endif()

message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})
add_subdirectory(src)

# ----------------------------------------------------------------------------
# Dependencies (Conda-only)
# ----------------------------------------------------------------------------

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
    find_package(OpenMP REQUIRED)
    target_link_libraries(opflow INTERFACE OpenMP::OpenMP_C OpenMP::OpenMP_CXX)
endif()

target_compile_definitions(opflow ${OPFLOW_TARGET_SCOPE} OPFLOW_WITH_HDF5 OPFLOW_WITH_VTK)
find_package(spdlog CONFIG REQUIRED)
target_link_libraries(opflow ${OPFLOW_TARGET_SCOPE} spdlog::spdlog)

find_package(amgcl CONFIG REQUIRED)
target_compile_definitions(opflow ${OPFLOW_TARGET_SCOPE} AMGCL_NO_BOOST)
target_link_libraries(opflow ${OPFLOW_TARGET_SCOPE} amgcl::amgcl)

find_package(TBB CONFIG REQUIRED)
target_link_libraries(opflow ${OPFLOW_TARGET_SCOPE} TBB::tbb)

find_package(HYPRE CONFIG REQUIRED)
target_link_libraries(opflow ${OPFLOW_TARGET_SCOPE} HYPRE::HYPRE)
include(CheckCSourceCompiles)
set(_opflow_saved_required_includes "${CMAKE_REQUIRED_INCLUDES}")
set(_opflow_saved_required_libraries "${CMAKE_REQUIRED_LIBRARIES}")
get_target_property(_opflow_hypre_includes HYPRE::HYPRE INTERFACE_INCLUDE_DIRECTORIES)
if(_opflow_hypre_includes AND NOT _opflow_hypre_includes STREQUAL "_opflow_hypre_includes-NOTFOUND")
    set(CMAKE_REQUIRED_INCLUDES "${_opflow_hypre_includes}")
endif()
set(CMAKE_REQUIRED_LIBRARIES HYPRE::HYPRE)
check_c_source_compiles(
    "#include <HYPRE_sstruct_ls.h>
int main(void) {
    (void)(&HYPRE_SStructFACCreate);
    return 0;
}
"
    OPFLOW_HYPRE_HAS_SSTRUCT_FAC)
set(CMAKE_REQUIRED_INCLUDES "${_opflow_saved_required_includes}")
set(CMAKE_REQUIRED_LIBRARIES "${_opflow_saved_required_libraries}")
if(OPFLOW_HYPRE_HAS_SSTRUCT_FAC)
    target_compile_definitions(opflow ${OPFLOW_TARGET_SCOPE} OPFLOW_HYPRE_HAS_SSTRUCT_FAC)
endif()

# TecIO (serial or MPI)
if(OPFLOW_WITH_MPI)
    find_package(teciompi CONFIG REQUIRED)
    target_compile_definitions(opflow ${OPFLOW_TARGET_SCOPE} TECIOMPI)
    target_link_libraries(opflow ${OPFLOW_TARGET_SCOPE} tecio::teciompi)
else()
    find_package(tecio CONFIG REQUIRED)
    target_link_libraries(opflow ${OPFLOW_TARGET_SCOPE} tecio::tecio)
endif()

# VTK is required (IO always on).
find_package(VTK CONFIG REQUIRED COMPONENTS CommonCore CommonDataModel IOXML)
target_link_libraries(opflow ${OPFLOW_TARGET_SCOPE} ${VTK_LIBRARIES})

# HDF5 is required (IO always on).
if(OPFLOW_WITH_MPI)
    set(HDF5_PREFER_PARALLEL TRUE)
endif()
# Prefer HDF5 in the active conda prefix to avoid mixing with host Homebrew installs.
if(DEFINED ENV{PREFIX})
    set(HDF5_ROOT "$ENV{PREFIX}")
elseif(DEFINED ENV{CONDA_PREFIX})
    set(HDF5_ROOT "$ENV{CONDA_PREFIX}")
endif()
if(HDF5_ROOT AND EXISTS "${HDF5_ROOT}/bin/h5cc")
    set(HDF5_C_COMPILER_EXECUTABLE "${HDF5_ROOT}/bin/h5cc")
endif()
find_package(HDF5 REQUIRED COMPONENTS C)
target_include_directories(opflow ${OPFLOW_TARGET_SCOPE} ${HDF5_INCLUDE_DIRS})
target_link_libraries(opflow ${OPFLOW_TARGET_SCOPE} ${HDF5_C_LIBRARIES})

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
    find_package(benchmark CONFIG REQUIRED)
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
    set(version_config_file "${CMAKE_CURRENT_BINARY_DIR}/opflowConfigVersion.cmake")
    set(export_dest_dir "${CMAKE_INSTALL_LIBDIR}/cmake/opflow")
    set(pkgconfig_install_dir "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
    set(pkg_config "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc")

    include(CMakePackageConfigHelpers)
    configure_package_config_file(
        "${project_config_in}"
        "${project_config_out}"
        INSTALL_DESTINATION "${export_dest_dir}"
    )

    write_basic_package_version_file("${version_config_file}" COMPATIBILITY SameMajorVersion)
    install(FILES "${project_config_out}" "${version_config_file}" DESTINATION "${export_dest_dir}")
    install(EXPORT opflow-targets
        FILE opflowTargets.cmake
        NAMESPACE opflow::
        DESTINATION "${export_dest_dir}")
    install(FILES "${CMAKE_BINARY_DIR}/Version.hpp" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

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

message(STATUS "")
set(_opflow_col_type_width 11)
set(_opflow_col_name_width 40)
set(_opflow_col_value_width 31)
set(_opflow_sep_line "+-------------+------------------------------------------+---------------------------------+")

function(_opflow_abbrev_token out token)
    set(_token "${token}")
    if(_token MATCHES "[/\\\\]")
        get_filename_component(_token "${_token}" NAME)
        if(_token STREQUAL "")
            set(_token "N/A")
        endif()
    endif()

    string(LENGTH "${_token}" _token_len)
    if(_token_len GREATER 24)
        string(SUBSTRING "${_token}" 0 21 _token_prefix)
        set(_token "${_token_prefix}...")
    endif()

    set(${out} "${_token}" PARENT_SCOPE)
endfunction()

function(_opflow_pad_or_truncate out value width)
    set(_value "${value}")
    string(REPLACE "\n" " " _value "${_value}")
    string(REPLACE "\r" " " _value "${_value}")
    string(REPLACE "\t" " " _value "${_value}")
    string(REPLACE "|" "/" _value "${_value}")
    string(REGEX REPLACE "  +" " " _value "${_value}")
    string(STRIP "${_value}" _value)

    string(LENGTH "${_value}" _len)
    if(_len GREATER width)
        math(EXPR _truncate_len "${width} - 3")
        if(_truncate_len GREATER 0)
            string(SUBSTRING "${_value}" 0 ${_truncate_len} _prefix)
            set(_value "${_prefix}...")
        else()
            set(_value "...")
        endif()
    else()
        set(_pad_count 0)
        math(EXPR _pad_count "${width} - ${_len}")
        set(_pad "")
        while(_pad_count GREATER 0)
            set(_pad "${_pad} ")
            math(EXPR _pad_count "${_pad_count} - 1")
        endwhile()
        set(_value "${_value}${_pad}")
    endif()
    set(${out} "${_value}" PARENT_SCOPE)
endfunction()

function(_opflow_render_row section name value)
    _opflow_pad_or_truncate(_section "${section}" "${_opflow_col_type_width}")
    _opflow_pad_or_truncate(_name "${name}" "${_opflow_col_name_width}")
    _opflow_pad_or_truncate(_value "${value}" "${_opflow_col_value_width}")
    message(STATUS "| ${_section} | ${_name} | ${_value} |")
endfunction()

function(_opflow_normalize_value out value)
    if("${value}" STREQUAL "" OR "${value}" STREQUAL "N/A")
        set(${out} "<UNSET>" PARENT_SCOPE)
        return()
    endif()

    if("${value}" MATCHES ";")
        set(_items "${value}")
        list(LENGTH _items _item_count)
        if(_item_count GREATER 5)
            list(GET _items 0 _first)
            list(GET _items 1 _second)
            math(EXPR _tail_index "${_item_count} - 1")
            list(GET _items ${_tail_index} _last)
            math(EXPR _remaining "${_item_count} - 3")
            _opflow_abbrev_token(_first_short "${_first}")
            _opflow_abbrev_token(_second_short "${_second}")
            _opflow_abbrev_token(_last_short "${_last}")
            set(${out} "${_first_short}, ${_second_short}, ... (+${_remaining} more), ${_last_short}" PARENT_SCOPE)
        else()
            set(_normalized "")
            foreach(_item IN LISTS _items)
                _opflow_abbrev_token(_short_item "${_item}")
                if(_normalized STREQUAL "")
                    set(_normalized "${_short_item}")
                else()
                    set(_normalized "${_normalized}, ${_short_item}")
                endif()
            endforeach()
            set(${out} "${_normalized}" PARENT_SCOPE)
        endif()
    else()
        _opflow_abbrev_token(_token_value "${value}")
        set(${out} "${_token_value}" PARENT_SCOPE)
    endif()
endfunction()

function(_opflow_dependency_root out value)
    set(_path "${value}")
    string(STRIP "${_path}" _path)
    if(_path STREQUAL "" OR _path STREQUAL "N/A")
        set(${out} "N/A" PARENT_SCOPE)
        return()
    endif()

    if(_path MATCHES "^INCLUDE:[[:space:]]*(.*)")
        set(_path "${CMAKE_MATCH_1}")
    endif()
    if(_path MATCHES "^-I(.*)")
        set(_path "${CMAKE_MATCH_1}")
    endif()

    if(IS_DIRECTORY "${_path}")
        set(_candidate "${_path}")
    else()
        if(NOT _path MATCHES "[/\\\\]")
            set(${out} "N/A" PARENT_SCOPE)
            return()
        endif()
        get_filename_component(_candidate "${_path}" DIRECTORY)
    endif()

    if(_candidate STREQUAL "" OR NOT EXISTS "${_candidate}")
        set(${out} "N/A" PARENT_SCOPE)
        return()
    endif()

    foreach(_segment IN ITEMS bin lib64 lib include cmake share)
        if(_candidate MATCHES "^(.+)[/\\\\]${_segment}([/\\\\].*)?$")
            set(_candidate "${CMAKE_MATCH_1}")
            break()
        endif()
    endforeach()

    set(${out} "${_candidate}" PARENT_SCOPE)
endfunction()

function(_opflow_normalize_dependency_location out value)
    if("${value}" STREQUAL "N/A" OR "${value}" MATCHES "-NOTFOUND$")
        set(${out} "N/A" PARENT_SCOPE)
        return()
    endif()

    if("${value}" MATCHES ";")
        set(_items "${value}")
        set(_result "")
        foreach(_item IN LISTS _items)
            _opflow_dependency_root(_candidate "${_item}")
            if(NOT _candidate STREQUAL "N/A")
                set(_result "${_candidate}")
                break()
            endif()
        endforeach()
    else()
        _opflow_dependency_root(_result "${value}")
    endif()

    if(_result STREQUAL "" OR _result STREQUAL "N/A")
        set(_result "N/A")
    endif()

    set(${out} "${_result}" PARENT_SCOPE)
endfunction()

function(_opflow_target_location out target)
    set(_result "N/A")
    if(TARGET "${target}")
        foreach(_prop IN ITEMS
                IMPORTED_LOCATION
                IMPORTED_LOCATION_RELEASE
                IMPORTED_LOCATION_DEBUG
                IMPORTED_IMPLIB
                IMPORTED_LOCATION_MINSIZEREL
                IMPORTED_LOCATION_RELWITHDEBINFO
                LOCATION)
            get_target_property(_loc "${target}" ${_prop})
            if(_loc AND NOT _loc MATCHES "NOTFOUND$")
                set(_result "${_loc}")
                break()
            endif()
        endforeach()
        if(_result STREQUAL "N/A")
            get_target_property(_inc "${target}" INTERFACE_INCLUDE_DIRECTORIES)
            if(_inc AND NOT _inc MATCHES "NOTFOUND$")
                set(_result "INCLUDE: ${_inc}")
            endif()
        endif()
    endif()
    set(${out} "${_result}" PARENT_SCOPE)
endfunction()

function(_opflow_print_row section name value)
    _opflow_normalize_value(_value "${value}")
    _opflow_render_row("${section}" "${name}" "${_value}")
endfunction()

function(_opflow_print_option name)
    if(DEFINED ${name})
        _opflow_print_row("Option" "${name}" "${${name}}")
    else()
        _opflow_print_row("Option" "${name}" "<UNSET>")
    endif()
endfunction()

function(_opflow_print_dependency name found location)
    if(found)
        set(_found "YES")
    else()
        set(_found "NO")
    endif()
    _opflow_normalize_dependency_location(_loc "${location}")
    _opflow_render_row("Dependency" "${name} [${_found}]" "${_loc}")
endfunction()

message(STATUS "${_opflow_sep_line}")
_opflow_render_row("Type" "Name" "Value/Location")
message(STATUS "${_opflow_sep_line}")

get_cmake_property(_opflow_option_vars VARIABLES)
list(FILTER _opflow_option_vars INCLUDE REGEX "^OPFLOW_")
list(SORT _opflow_option_vars)
foreach(_opflow_option_var IN LISTS _opflow_option_vars)
    _opflow_print_option("${_opflow_option_var}")
endforeach()

message(STATUS "${_opflow_sep_line}")
_opflow_render_row("Dependency" "Status" "Location")
message(STATUS "${_opflow_sep_line}")

if(OPFLOW_WITH_MPI)
    _opflow_target_location(_opflow_dep_mpi_path MPI::MPI_C)
    if(_opflow_dep_mpi_path STREQUAL "N/A")
        if(DEFINED MPI_C_COMPILER)
            _opflow_normalize_dependency_location(_opflow_dep_mpi_path "${MPI_C_COMPILER};${MPI_CXX_COMPILER};${MPI_C_LIBRARIES};${MPI_CXX_LIBRARIES};${MPI_C_INCLUDE_DIRS};${MPI_CXX_INCLUDE_DIRS}")
        elseif(DEFINED MPI_CXX_INCLUDE_DIRS)
            _opflow_normalize_dependency_location(_opflow_dep_mpi_path "${MPI_CXX_INCLUDE_DIRS}")
        endif()
    endif()
    _opflow_print_dependency("MPI (OPFLOW_WITH_MPI)" TRUE "${_opflow_dep_mpi_path}")
else()
    _opflow_print_dependency("MPI (OPFLOW_WITH_MPI)" FALSE "disabled")
endif()

if(OPFLOW_WITH_OPENMP)
    if(TARGET OpenMP::OpenMP_CXX)
        _opflow_target_location(_opflow_dep_openmp_path OpenMP::OpenMP_CXX)
    else()
        if(DEFINED OpenMP_CXX_FLAGS)
            _opflow_normalize_dependency_location(_opflow_dep_openmp_path "${OpenMP_CXX_FLAGS};${OpenMP_CXX_LIB_NAMES};${OpenMP_CXX_LIBRARIES};${OpenMP_CXX_INCLUDE_DIRS}")
        else()
            set(_opflow_dep_openmp_path "N/A")
        endif()
    endif()
    _opflow_print_dependency("OpenMP (OPFLOW_WITH_OPENMP)" TRUE "${_opflow_dep_openmp_path}")
else()
    _opflow_print_dependency("OpenMP (OPFLOW_WITH_OPENMP)" FALSE "disabled")
endif()

_opflow_target_location(_opflow_dep_spdlog_path spdlog::spdlog)
_opflow_print_dependency("spdlog" TRUE "${_opflow_dep_spdlog_path}")

_opflow_target_location(_opflow_dep_amgcl_path amgcl::amgcl)
_opflow_print_dependency("amgcl" TRUE "${_opflow_dep_amgcl_path}")

_opflow_target_location(_opflow_dep_tbb_path TBB::tbb)
_opflow_print_dependency("TBB" TRUE "${_opflow_dep_tbb_path}")

_opflow_target_location(_opflow_dep_hypre_path HYPRE::HYPRE)
_opflow_print_dependency("HYPRE" TRUE "${_opflow_dep_hypre_path}")

if(OPFLOW_WITH_MPI)
    _opflow_target_location(_opflow_dep_tecio_path tecio::teciompi)
    _opflow_print_dependency("tecio (MPI)" TRUE "${_opflow_dep_tecio_path}")
else()
    _opflow_target_location(_opflow_dep_tecio_path tecio::tecio)
    _opflow_print_dependency("tecio" TRUE "${_opflow_dep_tecio_path}")
endif()

_opflow_normalize_dependency_location(_opflow_dep_vtk_path "${VTK_LIBRARIES};${VTK_INCLUDE_DIRS};${VTK_CMAKE_DIR};${VTK_DIR}")
_opflow_print_dependency("VTK" TRUE "${_opflow_dep_vtk_path}")

_opflow_normalize_dependency_location(_opflow_dep_hdf5_path "${HDF5_C_LIBRARIES};${HDF5_INCLUDE_DIRS};${HDF5_C_INCLUDE_DIR};${HDF5_DIR}")
_opflow_print_dependency("HDF5" TRUE "${_opflow_dep_hdf5_path}")

if(OPFLOW_BUILD_BENCHMARKS OR OPFLOW_BUILD_ALL)
    _opflow_target_location(_opflow_dep_bench_path benchmark::benchmark)
    _opflow_print_dependency("benchmark" TRUE "${_opflow_dep_bench_path}")
endif()

if(OPFLOW_BUILD_DOCS OR OPFLOW_BUILD_ALL)
    _opflow_print_dependency("sphinx-build" TRUE "${SPHINX_PATH}")
    _opflow_print_dependency("doxygen" TRUE "${DOXYGEN_PATH}")
endif()

message(STATUS "${_opflow_sep_line}")
