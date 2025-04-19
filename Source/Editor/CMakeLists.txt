set(BEE_TARGET Editor)

file(GLOB_RECURSE SOURCE_FILES ./*.hpp ./*.cpp ./*.ui)
AssignSourceGroup(${SOURCE_FILES})

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

qt_add_executable(${BEE_TARGET} ${SOURCE_FILES})

SetCompilerFlags(${BEE_TARGET})
SetDefaultCompileDefinitions(${BEE_TARGET})

target_compile_features(${BEE_TARGET} PUBLIC cxx_std_23)

target_link_libraries(${BEE_TARGET}
        PRIVATE
#        Bee::Bee
        Qt6::Widgets
)

target_include_directories(${BEE_TARGET}
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(${BEE_TARGET} 
        PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY ${${PROJECT_NAME_UPPERCASE}_BINARY_DIR}
        WIN32_EXECUTABLE ON
)

BeeDeployQT(${BEE_TARGET})

qt_generate_deploy_app_script(
        TARGET ${BEE_TARGET}
        OUTPUT_SCRIPT DeployScript
        NO_UNSUPPORTED_PLATFORM_ERROR
)

install(SCRIPT ${DeployScript})
