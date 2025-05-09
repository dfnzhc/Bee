set(BEE_TARGET Bee)

file(GLOB_RECURSE SOURCE_FILES ./*.hpp ./*.cpp)
AssignSourceGroup(${SOURCE_FILES})

add_library(${BEE_TARGET} SHARED ${SOURCE_FILES})
BeeDefaultSettings(${BEE_TARGET})

target_link_libraries(${BEE_TARGET}
        PUBLIC 
        Mimalloc
        GLM
        PRIVATE 
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

target_compile_features(${BEE_TARGET} 
        PUBLIC 
        cxx_std_23
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

if (WIN32)
     add_custom_command(TARGET ${BEE_TARGET} 
             POST_BUILD COMMAND
            "${${PROJECT_NAME_UPPERCASE}_TOOLS_DIR}/minject.exe" --force --inplace $<$<CONFIG:Debug>:--postfix=debug> ${${PROJECT_NAME_UPPERCASE}_BINARY_DIR}/${BEE_TARGET}.dll
     )
endif (WIN32)