set(COMP_NAME Application)
set(IS_REQUIRED TRUE)
set(IS_CUDA_DEPEND FALSE)

set(TARGET_NAME "Bee${COMP_NAME}")
file(GLOB_RECURSE CompSources
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB_RECURSE CompCUDASources
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cu"
)

file(GLOB_RECURSE CompHeaders
        "${BEE_INCLUDE_DIR}/Bee/${COMP_NAME}/*.hpp"
        "${BEE_INCLUDE_DIR}/Bee/${COMP_NAME}/*.inl"
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
        Bee::Core
        Bee::Math
        Bee::Platform
)

BeeAddComponent(
        NAME ${COMP_NAME}
        SOURCES ${CompSources}
        HEADERS ${CompHeaders}
        CUDA_SOURCES ${CompCUDASources}
        CUDA_DEPEND ${IS_CUDA_DEPEND}
        COMPILE_DEFINITIONS ${CompDefinitions}
        COMPILE_OPTIONS ${CompOptions}
        INCLUDE_DIRECTORIES ${CompIncludeDirs}
        LINK_LIBRARIES ${CompLinkLibraries}
)

target_include_directories(${TARGET_NAME} 
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        "${BEE_INCLUDE_DIR}/Bee/${COMP_NAME}"
)

BeeLogComponent(${COMP_NAME} IS_REQUIRED ${IS_CUDA_DEPEND} "Application.")
