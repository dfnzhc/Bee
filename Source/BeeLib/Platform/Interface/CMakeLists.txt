set(LAYER_NAME Platform)
set(COMP_NAME Interface)

file(GLOB_RECURSE CompSources
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB_RECURSE CompHeaders
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.inl"
)

set(CompDefinitions
        $<$<CONFIG:Debug>:BEE_DEBUG=1>
)

set(CompOptions
)

set(CompIncludeDirs
)

set(CompLinkLibraries
        Bee::Core::Base
        Bee::Core::Math
)

BeeAddComponent(
        NAME ${COMP_NAME}
        LAYER ${LAYER_NAME}
        SOURCES ${CompSources}
        HEADERS ${CompHeaders}
        COMPILE_DEFINITIONS ${CompDefinitions}
        COMPILE_OPTIONS ${CompOptions}
        INCLUDE_DIRECTORIES ${CompIncludeDirs}
        LINK_LIBRARIES ${CompLinkLibraries}
)

set(COMP_LIB_NAME Bee${LAYER_NAME}_${COMP_NAME})

target_compile_definitions(${COMP_LIB_NAME} PRIVATE
)

get_target_property(TARGET_TYPE ${COMP_LIB_NAME} TYPE)
if(NOT TARGET_TYPE STREQUAL "INTERFACE_LIBRARY")
    BeeConfigurePrecompiledHeaders(
            TARGET_NAME ${COMP_LIB_NAME}
    )
endif()