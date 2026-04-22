file(GLOB_RECURSE event_public_headers CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

bee_add_component(
    NAME Event
    TYPE INTERFACE
    PUBLIC_HEADERS
        ${event_public_headers}
    DEPENDS Bee::Base Bee::Concurrency
)
