message(STATUS "----- Configuring Graphics Layer -----")

set(LAYER_NAME Graphics)

set(LayerSources
        Graphics.hpp Graphics.cpp
)

set(LayerDependencies
        Bee::Core
        Bee::Platform
)

set(LayerComponents
)

set(LayerDefinitions
)

set(LayerOptions
)

set(LayerIncludeDirs
        ${CMAKE_CURRENT_SOURCE_DIR}
)

BeeAddLayer(
        NAME ${LAYER_NAME}
        TYPE STATIC
        SOURCES "${LayerSources}"
        DEPENDENCIES ${LayerDependencies}
        COMPONENTS ${LayerComponents}
        COMPILE_DEFINITIONS ${LayerDefinitions}
        COMPILE_OPTIONS ${LayerOptions}
        INCLUDE_DIRECTORIES ${LayerIncludeDirs}
)

target_compile_definitions(Bee${LAYER_NAME} PRIVATE
)

target_compile_options(Bee${LAYER_NAME} PRIVATE
)

target_include_directories(Bee${LAYER_NAME} PRIVATE
)
