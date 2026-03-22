set(concurrency_library_type "STATIC")
set(concurrency_private_include_dir "${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB_RECURSE concurrency_public_headers CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)
file(GLOB_RECURSE concurrency_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)

bee_add_component(
    NAME Concurrency
    TYPE "${concurrency_library_type}"
    SOURCES
        ${concurrency_sources}
    PUBLIC_HEADERS
        ${concurrency_public_headers}
    PRIVATE_INCLUDE_DIRS
        "${concurrency_private_include_dir}"
    DEPENDS
        Bee::Base
)
