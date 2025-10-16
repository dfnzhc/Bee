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