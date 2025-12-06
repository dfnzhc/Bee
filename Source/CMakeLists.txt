
add_subdirectory(Core)
add_subdirectory(Container)
#add_subdirectory(Utils)

add_library(Bee INTERFACE)

target_link_libraries(Bee
        INTERFACE
        Bee::Core
        Bee::Container
)

target_include_directories(Bee
        INTERFACE
        ${BEE_INCLUDE_DIR}
)

add_library(Bee::Bee ALIAS Bee)
