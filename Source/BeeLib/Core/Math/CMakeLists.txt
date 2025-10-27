set(LAYER_NAME Core)
set(COMP_NAME Math)

file(GLOB_RECURSE CompSources
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB_RECURSE CompHeaders
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.inl"
)

set(CompDefinitions
)

set(CompOptions
)

set(CompIncludeDirs
)

set(CompLinkLibraries
        Bee::Core::Logger
        GLM
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

SetDefaultCompileDefinitions(${COMP_LIB_NAME})

target_compile_definitions(${COMP_LIB_NAME} PRIVATE
        BEE_LOG_TAG="${LAYER_LOG_TAG}"
)

target_link_libraries(${COMP_LIB_NAME} PRIVATE
)

get_target_property(TARGET_TYPE ${COMP_LIB_NAME} TYPE)
if (NOT TARGET_TYPE STREQUAL "INTERFACE_LIBRARY")
    BeeConfigurePrecompiledHeaders(
            TARGET_NAME ${COMP_LIB_NAME}
    )
endif ()