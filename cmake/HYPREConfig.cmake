# config HYPRE. Download & build if not previously installed
macro(CONFIG_HYPRE)
    include(MorseInit)
    if (OpFlow_with_openmp)
        set(CMAKE_REQUIRED_FLAGS "-fopenmp")
    endif ()
    find_package(HYPRE)
    if (HYPRE_FOUND)
        include_directories(${HYPRE_INCLUDE_DIRS})
        link_directories(${HYPRE_LIBRARY_DIRS})
    else ()
        #Build hypre and include
        configure_file(cmake/DownloadHYPRE.txt.in hypre-download/CMakeLists.txt)
        execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
                        RESULT_VARIABLE result
                        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/hypre-download)
        if (result)
            message(FATAL_ERROR "CMake step for HYPRE failed: ${result}")
        endif ()

        set(HYPRE_OPTIONS "-DHYPRE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DHYPRE_ENABLE_SINGLE=${OpFlow_single_precision} \
     -DHYPRE_WITH_OPENMP=${OpFlow_with_openmp} -DHYPRE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/hypre")

        message(STATUS "HYPRE is configed with: ${HYPRE_OPTIONS}")

        execute_process(COMMAND ${CMAKE_COMMAND} --build . -j
                        RESULT_VARIABLE result
                        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/hypre-download)
        if (result)
            message(FATAL_ERROR "Build step for HYPRE failed: ${result}")
        endif ()

        set(HYPRE_DIR ${CMAKE_CURRENT_BINARY_DIR}/hypre)
        if (OpFlow_with_openmp)
            set(CMAKE_REQUIRED_FLAGS "-fopenmp")
        endif ()
        find_package(HYPRE REQUIRED)
        include_directories(${HYPRE_INCLUDE_DIRS})
        link_directories(${HYPRE_LIBRARY_DIRS})
    endif ()
endmacro()