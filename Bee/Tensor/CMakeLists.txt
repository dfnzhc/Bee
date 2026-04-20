set(tensor_library_type "STATIC")
set(tensor_private_include_dir "${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB_RECURSE tensor_public_headers CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)
file(GLOB_RECURSE tensor_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)

bee_add_component(
    NAME Tensor
    TYPE "${tensor_library_type}"
    SOURCES
        ${tensor_sources}
    PUBLIC_HEADERS
        ${tensor_public_headers}
    PRIVATE_INCLUDE_DIRS
        "${tensor_private_include_dir}"
    DEPENDS
        Bee::Base
)
