project(OpFlow_Misc_Tests)

add_gmock(FmtlibTest ${CMAKE_CURRENT_SOURCE_DIR}/FmtlibTest.cpp)
target_link_libraries(FmtlibTest fmt::fmt)

add_gmock(SpdlogTest ${CMAKE_CURRENT_SOURCE_DIR}/SpdlogTest.cpp)