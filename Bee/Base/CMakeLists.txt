set(base_library_type "STATIC")
set(base_private_include_dir "${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB base_public_headers CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)
file(GLOB base_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)

bee_add_component(
    NAME Base
    TYPE "${base_library_type}"
    SOURCES
        ${base_sources}
    PUBLIC_HEADERS
        ${base_public_headers}
    PRIVATE_INCLUDE_DIRS
        "${base_private_include_dir}"
)

if(BEE_BUILD_TESTS)
    add_subdirectory(Tests)
endif()
