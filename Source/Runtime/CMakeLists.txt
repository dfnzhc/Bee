set(BEE_TARGET BeeRuntime)

file(GLOB_RECURSE SOURCE_FILES ./*.hpp ./*.cpp)
AssignSourceGroup(${SOURCE_FILES})

add_library(${BEE_TARGET} SHARED ${SOURCE_FILES})
BeeDefaultSettings(${BEE_TARGET})

find_package(fmt CONFIG REQUIRED)
find_package(libassert CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)

target_link_libraries(${BEE_TARGET}
        #    PRIVATE
        PUBLIC
        fmt::fmt
        libassert::assert
        glfw
        magic_enum::magic_enum
)

target_compile_options(${BEE_TARGET}
        PUBLIC
        # MSVC
        $<$<CXX_COMPILER_ID:MSVC>:/DEBUG>
)

target_include_directories(${BEE_TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Public)