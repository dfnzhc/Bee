# 日志 - spdlog
CPMAddPackage(
        NAME spdlog
        VERSION 1.16.0
        GITHUB_REPOSITORY gabime/spdlog
)

if (NOT spdlog_ADDED)
    message(FATAL_ERROR "Failed to add spdlog library.")
endif ()

add_library(Spdlog INTERFACE)
target_link_libraries(Spdlog INTERFACE spdlog::spdlog)
set_property(TARGET Spdlog PROPERTY FOLDER "External")

# sdl3
CPMAddPackage(
        NAME sdl3
        GITHUB_REPOSITORY "libsdl-org/SDL"
        GIT_TAG "release-3.2.24"
        OPTIONS
        "SDL_TESTS OFF"
        "SDL_EXAMPLES OFF"
        "SDL_SHARED OFF"
        "SDL_STATIC ON"
        "SDL_DISABLE_INSTALL_DOCS ON"
)

if (NOT sdl3_ADDED)
    message(FATAL_ERROR "Failed to add sdl3 library.")
endif ()

add_library(SDL3 INTERFACE)
target_link_libraries(SDL3 INTERFACE SDL3::SDL3-static)
set_property(TARGET SDL3 PROPERTY FOLDER "External")

# glm
CPMAddPackage(
        NAME glm
        GITHUB_REPOSITORY "g-truc/glm"
        GIT_TAG "1.0.2"
        OPTIONS
        "GLM_BUILD_TESTS OFF"
        "BUILD_SHARED_LIBS OFF"
)

if (NOT glm_ADDED)
    message(FATAL_ERROR "Failed to add glm library.")
endif ()

add_library(GLM INTERFACE)
target_link_libraries(GLM INTERFACE glm::glm)
target_compile_definitions(GLM INTERFACE
        GLM_FORCE_SWIZZLE
        GLM_FORCE_DEPTH_ZERO_TO_ONE
        GLM_FORCE_RADIANS
        GLM_ENABLE_EXPERIMENTAL
)
set_property(TARGET GLM PROPERTY FOLDER "External")
