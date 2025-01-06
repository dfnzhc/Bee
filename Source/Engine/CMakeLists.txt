set(BEE_TARGET BeeEngine)

file(GLOB_RECURSE SOURCE_FILES ./*.hpp ./*.cpp)
AssignSourceGroup(${SOURCE_FILES})

add_library(${BEE_TARGET} SHARED ${SOURCE_FILES})
BeeDefaultSettings(${BEE_TARGET})

find_package(SDL3 CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

target_link_libraries(${BEE_TARGET}
        #    PRIVATE
        PUBLIC
        Bee::Runtime
        SDL3::SDL3
        volk
        GPUOpen::VulkanMemoryAllocator
)

target_compile_options(${BEE_TARGET}
        PUBLIC
        # MSVC
        $<$<CXX_COMPILER_ID:MSVC>:/DEBUG>
)

target_include_directories(${BEE_TARGET} 
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Private
)