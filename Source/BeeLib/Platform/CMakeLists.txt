message(STATUS "----- Configuring Platform Layer -----")

set(LAYER_NAME Platform)
set(LAYER_LOG_TAG 🖥️)

set(LayerSources
        Platform.hpp Platform.cpp
)

set(LayerDependencies
        Bee::Core
)

set(LayerComponents
        Interface
        Impl
)

set(LayerDefinitions
)

set(LayerOptions
)

set(LayerIncludeDirs
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
