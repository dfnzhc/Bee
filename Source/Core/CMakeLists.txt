set(BEE_TARGET Core)

file(GLOB_RECURSE SOURCE_FILES ./*.hpp ./*.cpp)
AssignSourceGroup(${SOURCE_FILES})

add_library(${BEE_TARGET} SHARED ${SOURCE_FILES})
BeeDefaultSettings(${BEE_TARGET})


find_package(fmt CONFIG REQUIRED)
find_package(libassert CONFIG REQUIRED)

target_link_libraries(${BEE_TARGET}
        #    PRIVATE
        PUBLIC
        fmt::fmt
        libassert::assert
)

target_compile_options(${BEE_TARGET}
        PUBLIC
        # MSVC
        $<$<CXX_COMPILER_ID:MSVC>:/DEBUG>
)

target_include_directories(${BEE_TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Public)