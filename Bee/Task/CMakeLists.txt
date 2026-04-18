# Task 是纯头文件库（所有实现均位于 .hpp 中），改用 INTERFACE 类型，
# 避免历史上残留的 STATIC .lib 与新头文件 ABI 不匹配导致的链接顺序问题。
file(GLOB_RECURSE task_public_headers CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

bee_add_component(
    NAME Task
    TYPE "INTERFACE"
    PUBLIC_HEADERS
        ${task_public_headers}
    DEPENDS
        Bee::Base Bee::Concurrency
)
