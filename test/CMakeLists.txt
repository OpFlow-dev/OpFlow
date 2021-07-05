cmake_minimum_required(VERSION 3.10)
project(OpFlow_Tests)

define_property(GLOBAL PROPERTY all_test_srcs BRIEF_DOCS "All test srcs" FULL_DOCS "All test srcs")
set_property(GLOBAL PROPERTY all_test_srcs)

function(add_gtest testname testfile)
    add_executable(${testname} ${testfile} ${ARGN})
    target_link_libraries(${testname} gtest_main ${OpFlow_Libs})
    set_property(GLOBAL APPEND PROPERTY all_test_srcs ${testfile} ${ARGN})
    if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        SETUP_TARGET_FOR_COVERAGE(${testname}_coverage ${testname} ${testname}_coverage)
    endif ()
endfunction()

function(add_gmock testname testfile)
    add_executable(${testname} ${testfile} ${ARGN})
    target_link_libraries(${testname} gmock_main fmt::fmt ${OpFlow_Libs})
    set_property(GLOBAL APPEND PROPERTY all_test_srcs ${testfile} ${ARGN})
    if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        SETUP_TARGET_FOR_COVERAGE(${testname}_coverage ${testname} ${testname}_coverage)
    endif ()
endfunction()

if (OpFlow_build_tests)
    # Download and unpack googletest at configure time
    configure_file(../cmake/DownloadGTest.txt.in googletest-download/CMakeLists.txt)
    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
            RESULT_VARIABLE result
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download)
    if (result)
        message(FATAL_ERROR "CMake step for googletest failed: ${result}")
    endif ()
    execute_process(COMMAND ${CMAKE_COMMAND} --build .
            RESULT_VARIABLE result
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download)
    if (result)
        message(FATAL_ERROR "Build step for googletest failed: ${result}")
    endif ()

    # Prevent overriding the parent project's compiler/linker
    # settings on Windows
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    # Add googletest directly to our build. This defines
    # the gtest and gtest_main targets.
    add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
            ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
            EXCLUDE_FROM_ALL)

    # The gtest/gtest_main targets carry header search path
    # dependencies automatically when using CMake 2.8.11 or
    # later. Otherwise we have to add them here ourselves.
    if (CMAKE_VERSION VERSION_LESS 2.8.11)
        include_directories("${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
    endif ()

    include_directories(${CMAKE_CURRENT_BINARY_DIR})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")

    add_subdirectory(Core)
    add_subdirectory(DataStructures)
    add_subdirectory(Misc)
    add_subdirectory(Utils)

    # Add global tests target
    get_property(all_test_srcs_list GLOBAL PROPERTY all_test_srcs)
    list(REMOVE_DUPLICATES all_test_srcs_list)
    add_gmock(AllTests ${all_test_srcs_list})
    target_link_libraries(AllTests iomp5 pthread fmt::fmt ${OpFlow_Libs})
endif ()