set(COMP_NAME Editor)
set(TARGET_NAME "Bee${COMP_NAME}")

file(GLOB_RECURSE SOURCE_FILES ./*.hpp ./*.cpp)
AssignSourceGroup(${SOURCE_FILES})

add_executable(${TARGET_NAME} ${SOURCE_FILES})
target_link_libraries(${TARGET_NAME}
        PRIVATE
        Bee::Bee
        Bee::EntryPoint
)

target_include_directories(${TARGET_NAME}
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(${TARGET_NAME}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${BEE_BINARY_DIR}
)

target_compile_definitions(${TARGET_NAME}
        PRIVATE
        BEE_LOG_TAG="🖥️"
)

string(LENGTH "${COMP_NAME}" NAME_LEN)
math(EXPR PAD_LEN "23 - ${NAME_LEN}") # 调整宽度以适应较长的别名
if (PAD_LEN LESS 0)
    set(PAD_LEN 1)
endif ()
string(REPEAT "." ${PAD_LEN} DOTS)
message(STATUS "${LOG_TREE_MID}${COMP_NAME}${C_Gray}${DOTS}${C_Reset} ${C_White}[Executable]${C_Reset}: Editor of the Bee Engine.")