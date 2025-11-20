file(GLOB_RECURSE SOURCE_FILES ./*.hpp ./*.cpp)
AssignSourceGroup(SOURCE_FILES)

add_library(Bee STATIC ${SOURCE_FILES})
add_library(Bee::Bee ALIAS Bee)

target_include_directories(Bee
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(Bee
        PUBLIC
        cxx_std_23
)

target_link_libraries(Bee
        PUBLIC

        PRIVATE
)

target_compile_definitions(Bee
        PUBLIC
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

        PRIVATE
        # Clang.
        $<$<CXX_COMPILER_ID:Clang>:_MSC_EXTENSIONS>
)

# TODO: 其他编译器设置 UTF-8
target_compile_options(Bee
        PUBLIC
        $<$<C_COMPILER_ID:MSVC>:/utf-8>
        $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
        PRIVATE
)

set_target_properties(Bee PROPERTIES
        EXPORT_NAME Bee
        OUTPUT_NAME BeeLib
)
