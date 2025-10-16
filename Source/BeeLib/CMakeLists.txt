message(STATUS "")
message(STATUS "=== Configuring Bee Engine Library ===")

message(STATUS "Adding Bee Engine layers...")

set(BEE_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(Core)
add_subdirectory(Platform)
add_subdirectory(Graphics)
add_subdirectory(Engine)
add_subdirectory(App)

add_library(BeeLib INTERFACE)

# 设置目标属性
set_target_properties(BeeLib PROPERTIES
        EXPORT_NAME Bee
        FOLDER "Bee"
        RUNTIME_OUTPUT_DIRECTORY ${BEE_BINARY_DIR}
)

target_link_libraries(BeeLib
        INTERFACE
        Bee::App
        Bee::Engine
        Bee::Graphics
        Bee::Platform
        Bee::Core
)

target_include_directories(BeeLib
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_compile_features(BeeLib
        INTERFACE
        cxx_std_23
)

add_library(Bee::Bee ALIAS BeeLib)

message(STATUS "=== BeeLib Configured Successfully ===")
message(STATUS "")
