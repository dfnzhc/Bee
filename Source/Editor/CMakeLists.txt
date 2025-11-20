file(GLOB_RECURSE SOURCE_FILES ./*.hpp ./*.cpp)
AssignSourceGroup(${SOURCE_FILES})

add_executable(Editor ${SOURCE_FILES})

target_link_libraries(Editor
        PRIVATE
        Bee::Bee
)

target_include_directories(Editor
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(Editor
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${BEE_BINARY_DIR}
)

target_compile_definitions(Editor PRIVATE
)
