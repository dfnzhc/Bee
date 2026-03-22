set(data_structure_library_type "STATIC")
set(data_structure_private_include_dir "${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB_RECURSE data_structure_public_headers CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)
file(GLOB_RECURSE data_structure_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)

bee_add_component(
    NAME DataStructure
    TYPE "${data_structure_library_type}"
    SOURCES
        ${data_structure_sources}
    PUBLIC_HEADERS
        ${data_structure_public_headers}
    PRIVATE_INCLUDE_DIRS
        "${data_structure_private_include_dir}"
    DEPENDS
        Bee::Base
)
