file(GLOB_RECURSE SOURCE_FILES ./*.hpp ./*.cpp)
AssignSourceGroup(${SOURCE_FILES})

add_executable(BeeEditor ${SOURCE_FILES})

SetCompilerFlags(BeeEditor)
SetDefaultCompileDefinitions(BeeEditor)

set(EDITOR_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Vulkan REQUIRED)

target_link_libraries(BeeEditor
        PRIVATE
        Bee::Bee
        Vulkan::Vulkan
)

target_include_directories(BeeEditor
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/Dirty
)

set_target_properties(BeeEditor
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${BEE_BINARY_DIR}
)

target_compile_definitions(BeeEditor PRIVATE
        EDITOR_SOURCE_DIR="${EDITOR_SOURCE_DIR}"
)