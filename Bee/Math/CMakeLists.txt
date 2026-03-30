set(math_library_type "INTERFACE")

file(GLOB_RECURSE math_public_headers CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

bee_add_component(
    NAME Math
    TYPE "${math_library_type}"
    PUBLIC_HEADERS
        ${math_public_headers}
)
