# config VTK. Download & build if not previously installed
macro(CONFIG_VTK)
    find_package(VTK)
    if (VTK_FOUND)
        include_directories(${VTK_INCLUDE_DIRS})
        link_directories(${VTK_LIBRARY_DIRS})
    else ()
        #Build VTK and include
        configure_file(cmake/DownloadVTK.txt.in vtk-download/CMakeLists.txt)
        execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
                        RESULT_VARIABLE result
                        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/vtk-download)
        if (result)
            message(FATAL_ERROR "CMake step for VTK failed: ${result}")
        endif ()

        execute_process(COMMAND ${CMAKE_COMMAND} --build . -j
                        RESULT_VARIABLE result
                        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/vtk-download)
        if (result)
            message(FATAL_ERROR "Build step for VTK failed: ${result}")
        endif ()

        set(VTK_DIR ${CMAKE_CURRENT_BINARY_DIR}/vtk/lib/cmake/vtk-9.0)
        find_package(VTK REQUIRED)
        include_directories(${VTK_INCLUDE_DIRS})
        link_directories(${VTK_LIBRARY_DIRS})
    endif ()
endmacro()