# vcpkg
if (NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{CMAKE_TOOLCHAIN_FILE})
    set(CMAKE_TOOLCHAIN_FILE $ENV{CMAKE_TOOLCHAIN_FILE})
endif ()

# vulkan
find_package(Vulkan REQUIRED)


# volk
# set(VOLK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/volk")
# set(VOLK_FILES
#     "${VOLK_DIR}/volk.c"
#     "${VOLK_DIR}/volk.h")

# add_library(volk STATIC ${VOLK_FILES})
# set_target_properties(volk PROPERTIES POSITION_INDEPENDENT_CODE ON)

# target_link_libraries(volk PUBLIC Vulkan::Vulkan)
# target_include_directories(volk SYSTEM PUBLIC ${VOLK_DIR})

# set_property(TARGET volk PROPERTY FOLDER "External")