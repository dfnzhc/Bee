# vcpkg
if (NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{CMAKE_TOOLCHAIN_FILE})
    set(CMAKE_TOOLCHAIN_FILE $ENV{CMAKE_TOOLCHAIN_FILE})
endif ()

# Helper macros
macro(ConfigureInterfaceLibrary LIB_NAME LINK_NAME TARGET_NAME)
    find_package(${LIB_NAME} CONFIG REQUIRED)
    add_library(${TARGET_NAME} INTERFACE)
    target_link_libraries(${TARGET_NAME} INTERFACE ${LINK_NAME})
    set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER "External")
endmacro()

macro(ConfigurePathLibrary INCLUDE_VAR HEADER_PATH TARGET_NAME)
    find_path(${INCLUDE_VAR} ${HEADER_PATH})
    add_library(${TARGET_NAME} INTERFACE)
    target_include_directories(${TARGET_NAME} INTERFACE ${${INCLUDE_VAR}})
    set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER "External")
endmacro()

# mimalloc

ConfigureInterfaceLibrary(mimalloc mimalloc-static Mimalloc)
if(MSVC)
	target_compile_options(Mimalloc INTERFACE /INCLUDE:mi_version)
endif()

#find_package(mimalloc CONFIG REQUIRED)

#add_library(Mimalloc INTERFACE)
#target_link_libraries(Mimalloc INTERFACE mimalloc-static)
#if(MSVC)
#	target_compile_options(Mimalloc INTERFACE /INCLUDE:mi_version)
#endif()

#set_property(TARGET Mimalloc PROPERTY FOLDER "External")


# magic_enum
ConfigureInterfaceLibrary(magic_enum magic_enum::magic_enum MagicEnum)

# quill
ConfigureInterfaceLibrary(quill quill::quill Quill)

# eventpp
ConfigureInterfaceLibrary(eventpp eventpp::eventpp Eventpp)

# stringzilla
ConfigurePathLibrary(STRINGZILLA_INCLUDE_DIRS "stringzilla/drafts.h" Stringzilla)

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