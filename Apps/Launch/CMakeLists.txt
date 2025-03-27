set(BEE_TARGET Launch)

file(GLOB_RECURSE SOURCE_FILES ./*.hpp ./*.cpp)
AssignSourceGroup(${SOURCE_FILES})

add_executable(${BEE_TARGET} ${SOURCE_FILES})

SetCompilerFlags(${BEE_TARGET})
SetDefaultCompileDefinitions(${BEE_TARGET})

target_compile_features(${BEE_TARGET} PUBLIC cxx_std_23)

target_link_libraries(${BEE_TARGET}
        PUBLIC
        Bee::Bee
        PRIVATE
        ${Slang_LIBRARY}
)

target_include_directories(${BEE_TARGET}
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(${BEE_TARGET} 
        PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY ${${PROJECT_NAME_UPPERCASE}_BINARY_DIR})
