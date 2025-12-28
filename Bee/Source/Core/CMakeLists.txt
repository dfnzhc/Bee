set(COMP_NAME Core)
set(IS_REQUIRED TRUE)
set(IS_CUDA_DEPEND TRUE)

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
        $<$<CONFIG:Release>:NDEBUG>
        $<$<CONFIG:Debug>:_DEBUG>
        # Windows
        $<$<PLATFORM_ID:Windows>:NOMINMAX>
        $<$<PLATFORM_ID:Windows>:UNICODE>
        # MSVC C++ 
        $<$<CXX_COMPILER_ID:MSVC>:_USE_MATH_DEFINES>
        $<$<CXX_COMPILER_ID:MSVC>:_SCL_SECURE_NO_WARNINGS>
        $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
        $<$<CXX_COMPILER_ID:MSVC>:_ENABLE_EXTENDED_ALIGNED_STORAGE>
        $<$<CXX_COMPILER_ID:MSVC>:_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING>
        #
        $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>: BEE_DEBUG_BUILD>
        
)

set(CompOptions
        $<$<COMPILE_LANGUAGE:CUDA>:--std=c++20;--use_fast_math;--expt-relaxed-constexpr;--extended-lambda;--forward-unknown-to-host-compiler;-Xcompiler=/Zc:preprocessor>
        $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/Zc:preprocessor>
)

set(CompIncludeDirs
)

set(CompLinkLibraries
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

BeeLogComponent(${COMP_NAME} IS_REQUIRED IS_CUDA_DEPEND "Declarations and defines.")