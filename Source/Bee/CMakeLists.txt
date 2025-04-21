set(BEE_TARGET Bee)

file(GLOB_RECURSE SOURCE_FILES ./*.hpp ./*.cpp)
AssignSourceGroup(${SOURCE_FILES})

add_library(${BEE_TARGET} SHARED ${SOURCE_FILES})
BeeDefaultSettings(${BEE_TARGET})

target_link_libraries(${BEE_TARGET} 
        PRIVATE 
        Mimalloc
        Stringzilla
        Eventpp
)

target_link_libraries(${BEE_TARGET}
        PUBLIC
        MagicEnum
        Quill
)

target_compile_options(${BEE_TARGET}
        PUBLIC
        # MSVC
        $<$<CXX_COMPILER_ID:MSVC>:/DEBUG /Zc:preprocessor>
)

target_compile_definitions(${BEE_TARGET}
        PUBLIC
        "$<$<CONFIG:Debug,RelWithDebInfo>:BEE_ENABLE_DEBUG>"
        PRIVATE
        ${PROJECT_NAME_UPPERCASE}_DLL
)

target_include_directories(${BEE_TARGET}
        PUBLIC
        ${CMAKE_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE
)