set(BEE_TARGET Bee)

file(GLOB_RECURSE SOURCE_FILES ./*.hpp ./*.cpp)
AssignSourceGroup(${SOURCE_FILES})

add_library(${BEE_TARGET} SHARED ${SOURCE_FILES})
BeeDefaultSettings(${BEE_TARGET})

target_precompile_headers(${BEE_TARGET}
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public/Bee.hpp
)

find_package(fmt CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_path(STRINGZILLA_INCLUDE_DIRS "stringzilla/drafts.h")

target_link_libraries(${BEE_TARGET}
        PUBLIC
        fmt::fmt
        magic_enum::magic_enum
        volk
        SDL3::SDL3
        GPUOpen::VulkanMemoryAllocator
        PRIVATE
        ${Slang_LIBRARY}
)

target_compile_options(${BEE_TARGET}
        PUBLIC
        # MSVC
        $<$<CXX_COMPILER_ID:MSVC>:/DEBUG /Zc:preprocessor>
)

target_compile_definitions(${BEE_TARGET}
        PUBLIC 
        "$<$<CONFIG:Debug,RelWithDebInfo>:BEE_ENABLE_DEBUG>"
)

target_include_directories(${BEE_TARGET}
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
        ${STRINGZILLA_INCLUDE_DIRS}
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
        ${SLANG_SDK_INCLUDES_DIR}
)
