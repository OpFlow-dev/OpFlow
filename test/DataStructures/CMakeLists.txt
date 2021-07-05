project(OpFlow_DS_Tests)

# MDArray
add_gmock(PlainTensorTest ${CMAKE_CURRENT_SOURCE_DIR}/Arrays/PlainTensorTest.cpp)