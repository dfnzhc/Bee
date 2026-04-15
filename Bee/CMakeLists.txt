cmake_minimum_required(VERSION 3.24)

set(BEE_PUBLIC_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

include(BeeComponent)

set(bee_component_targets)

# Automatically discover all component directories in Bee/* containing CMakeLists.txt.
file(GLOB component_dirs RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*")
foreach(component_dir IN LISTS component_dirs)
    set(component_cmake "${CMAKE_CURRENT_SOURCE_DIR}/${component_dir}/CMakeLists.txt")
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${component_dir}" AND EXISTS "${component_cmake}")
        add_subdirectory("${component_dir}")
        if(TARGET Bee::${component_dir})
            list(APPEND bee_component_targets Bee::${component_dir})
        endif()
    endif()
endforeach()

add_library(Bee INTERFACE)
add_library(Bee::Bee ALIAS Bee)
set_target_properties(Bee PROPERTIES FOLDER "Bee")

target_include_directories(Bee
    INTERFACE
        $<BUILD_INTERFACE:${BEE_PUBLIC_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>
)

if(bee_component_targets)
    target_link_libraries(Bee INTERFACE ${bee_component_targets})
endif()
