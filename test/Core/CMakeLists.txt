project(OpFlow_Core_Tests)

# Metas
add_gmock(MetaTest ${CMAKE_CURRENT_SOURCE_DIR}/MetaTest.cpp)

# Meshes
add_gmock(CartesianMeshTest ${CMAKE_CURRENT_SOURCE_DIR}/Mesh/CartesianMeshTest.cpp)

# Interpolators
add_gmock(InterpolatorTest ${CMAKE_CURRENT_SOURCE_DIR}/InterpolatorTest.cpp)

# RangeFor
add_gmock(RangeForTest ${CMAKE_CURRENT_SOURCE_DIR}/RangeForTest.cpp)

# MDIndex
add_gmock(MDIndexTest ${CMAKE_CURRENT_SOURCE_DIR}/MDIndexTest.cpp)
