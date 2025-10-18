message(STATUS "----- Configuring App Layer -----")

set(LAYER_NAME App)
set(LAYER_LOG_TAG 🎮)

set(LayerSources
        App.hpp App.cpp
)

set(LayerDependencies
        Bee::Core
        Bee::Platform
        Bee::Graphics
        Bee::Engine
)

set(LayerComponents
        Framework
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
