# ----------------------------------------------------------------------------
# CMakeLists.txt for the opflow library
# ----------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.16)

file(GLOB_RECURSE OPFLOW_HEADERS
    ${CMAKE_SOURCE_DIR}/src/*.hpp)
list(APPEND OPFLOW_HEADERS
    ${CMAKE_SOURCE_DIR}/include/Core
    ${CMAKE_SOURCE_DIR}/include/Math
    ${CMAKE_SOURCE_DIR}/include/DS
    ${CMAKE_SOURCE_DIR}/include/Utils
    ${CMAKE_SOURCE_DIR}/include/Version
    ${CMAKE_SOURCE_DIR}/include/OpFlow)

set(CMAKE_CXX_FLAGS_DEBUG "-Og -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

if(OPFLOW_ENABLE_MODULE)
    # ----------------------------------------------------------------------------
    # C++20 module version
    # ----------------------------------------------------------------------------
    add_library(opflow)

    target_compile_definitions(opflow INTERFACE OPFLOW_USE_MODULE)
    set_property(TARGET opflow PROPERTY CXX_SCAN_FOR_MODULES ON)
    target_sources(opflow
        PUBLIC FILE_SET CXX_MODULES
        FILES ${CMAKE_SOURCE_DIR}/src/opflow.cppm)
    target_sources(opflow
        PUBLIC FILE_SET HEADERS
        BASE_DIRS
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
        FILES ${OPFLOW_HEADERS})
    target_compile_features(opflow PUBLIC cxx_std_26)
    install(TARGETS opflow
        EXPORT opflow-targets
        FILE_SET CXX_MODULES DESTINATION ${CMAKE_INSTALL_LIBDIR}/cxx/opflow
        FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
else()
    # ----------------------------------------------------------------------------
    # C++20 header only version
    # ----------------------------------------------------------------------------
    add_library(opflow INTERFACE)
    set_property(TARGET opflow PROPERTY CXX_SCAN_FOR_MODULES OFF)
    target_compile_features(opflow INTERFACE cxx_std_20)
    target_sources(opflow
        INTERFACE FILE_SET HEADERS ${OPFLOW_HEADERS})
    install(TARGETS opflow
        EXPORT opflow-targets)
    install(FILES ${OPFLOW_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/opflow)
endif()
add_library(opflow::opflow ALIAS opflow)

# Create & include the version header
configure_file(${CMAKE_CURRENT_LIST_DIR}/../cmake/Version.hpp.in ${CMAKE_BINARY_DIR}/Version.hpp @ONLY)

target_include_directories(opflow PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../src>"
    "$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/src>")

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
