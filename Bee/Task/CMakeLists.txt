set(task_library_type "STATIC")
set(task_private_include_dir "${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB_RECURSE task_public_headers CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)
file(GLOB_RECURSE task_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)

bee_add_component(
    NAME Task
    TYPE "${task_library_type}"
    SOURCES
        ${task_sources}
    PUBLIC_HEADERS
        ${task_public_headers}
    PRIVATE_INCLUDE_DIRS
        "${task_private_include_dir}"
    DEPENDS
        Bee::Base Bee::Concurrency
)
