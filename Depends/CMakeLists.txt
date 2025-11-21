# Otter
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Otter/CMake PARENT_SCOPE)
include(HelperScripts)

set(OTT_BUILD_TESTS OFF)
add_subdirectory(Otter)

