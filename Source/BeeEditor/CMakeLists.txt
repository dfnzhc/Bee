file(GLOB_RECURSE SOURCE_FILES ./*.hpp ./*.cpp)
AssignSourceGroup(${SOURCE_FILES})

add_executable(BeeEditor ${SOURCE_FILES})

SetCompilerFlags(BeeEditor)
SetDefaultCompileDefinitions(BeeEditor)

target_link_libraries(BeeEditor
        PRIVATE
        Bee::Bee
)

target_include_directories(BeeEditor
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(BeeEditor
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${BEE_BINARY_DIR}
)