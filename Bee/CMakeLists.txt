file(GLOB ROOT_SOURCE *.cpp)
file(GLOB ROOT_INLINE *.inl)
file(GLOB ROOT_HEADER *.hpp)

file(GLOB_RECURSE DETAIL_SOURCE ./detail/**.cpp)
file(GLOB_RECURSE DETAIL_INLINE ./detail/**.inl)
file(GLOB_RECURSE DETAIL_HEADER ./detail/**.hpp)

source_group("Root Files" FILES ${ROOT_SOURCE})
source_group("Root Files" FILES ${ROOT_INLINE})
source_group("Root Files" FILES ${ROOT_HEADER})
source_group("Detail Files" FILES ${DETAIL_SOURCE})
source_group("Detail Files" FILES ${DETAIL_INLINE})
source_group("Detail Files" FILES ${DETAIL_HEADER})

add_library(bee-header-only INTERFACE)
add_library(bee::bee-header-only ALIAS bee-header-only)

target_include_directories(bee-header-only INTERFACE
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

if (BEE_BUILD_LIBRARY)
    add_library(bee
            ${ROOT_SOURCE} ${ROOT_INLINE} ${ROOT_HEADER}
            ${DETAIL_SOURCE} ${DETAIL_INLINE} ${DETAIL_HEADER}
    )
    add_library(bee::bee ALIAS bee)
    target_link_libraries(bee PUBLIC bee-header-only)
else ()
    add_library(bee INTERFACE)
    add_library(bee::bee ALIAS bee)
    target_link_libraries(bee INTERFACE bee-header-only)
endif ()

target_compile_features(bee
        PUBLIC cxx_std_20
        PRIVATE cxx_std_20
)

set_target_properties(bee PROPERTIES LINKER_LANGUAGE CXX)
