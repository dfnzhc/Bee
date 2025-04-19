set(BEE_TARGET Bee)

file(GLOB_RECURSE SOURCE_FILES ./*.hpp ./*.cpp)
AssignSourceGroup(${SOURCE_FILES})

add_library(${BEE_TARGET} SHARED ${SOURCE_FILES})
BeeDefaultSettings(${BEE_TARGET})

find_package(magic_enum CONFIG REQUIRED)
#find_package(SDL3 CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(quill CONFIG REQUIRED)
find_package(eventpp CONFIG REQUIRED)
find_path(STRINGZILLA_INCLUDE_DIRS "stringzilla/drafts.h")

target_link_libraries(${BEE_TARGET}
        PUBLIC
        quill::quill
        magic_enum::magic_enum
#        SDL3::SDL3
        eventpp::eventpp
        GPUOpen::VulkanMemoryAllocator
        PRIVATE
        #${Slang_LIBRARY}
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
        ${STRINGZILLA_INCLUDE_DIRS}
        PRIVATE
        #${SLANG_SDK_INCLUDES_DIR}
)