function(SetCompilerFlags ProjectName)
    if (NOT TARGET ${ProjectName})
        message(AUTHOR_WARNING "${ProjectName} is not a target, thus no compiler warning were added.")
        return()
    endif ()

    set(MSVC_FLAGS
            /Zi                                   # generate debug symbols
            /WX                                # warnings as errors
            /W4                                # increase warning level
            /wd4251                         # 'type' : class 'type1' needs to have dll-interface to be used by clients of class 'type2'
            /wd4244                         # 'conversion' conversion from 'type1' to 'type2', possible loss of data
            /wd4267                         # 'var' : conversion from 'size_t' to 'type', possible loss of data
            /wd4100                         # unreferenced formal parameter
            /wd4201                         # nonstandard extension used: nameless struct/union
            /wd4245                         # conversion from 'type1' to 'type2', signed/unsigned mismatch
            /wd4189                         # local variable is initialized but not referenced
            /wd4127                         # conditional expression is constant
            /wd4701                         # potentially uninitialized local variable 'name' used
            /wd4703                         # potentially uninitialized local pointer variable 'name' used
            /wd4324                         # structure was padded due to alignment specifier
            /wd4505                         # unreferenced local function has been removed
            /wd4702                         # unreachable code
            /wd4389                         # signed/unsigned mismatch
            /wd4459                         # declaration of 'identifier' hides global declaration
            /wd4268                         # 'identifier' : 'const' static/global data initialized with compiler generated default constructor fills the object with zeros
            /MP                                 # enable multi-processor compilation
    )

    set(CLANG_GCC_FLAGS
            -fms-extensions                 # enable MS extensions (among other things allow anonymous structs)
            -Wall                                   # set warning level
            -Wno-unused-function
            -Wno-unused-variable
            -Wno-unused-but-set-variable
            -Wno-switch
            -Wno-missing-braces
            -Wno-invalid-offsetof
    )

    set(CLANG_FLAGS
            -Wno-deprecated
            -Wno-newline-eof
            -Wno-c++98-compat
            -Wno-c++98-compat-pedantic
            -Wno-c++20-compat
            -Wno-old-style-cast
            -Wno-unused-function
            -Wno-double-promotion
            -Wno-builtin-macro-redefined
            -Wno-deprecated-declarations
            -Wno-unused-private-field
            -Wno-braced-scalar-init
            -Wno-switch-default
            -Wno-global-constructors
            -Wno-missing-prototypes
            -Wno-exit-time-destructors
            -Wno-self-assign-overloaded
            -Wno-ctad-maybe-unsupported
            -Wno-undef
            -Wno-unreachable-code-return
    )

    set(GCC_FLAGS
            -fpermissive
            -Wno-sign-compare
            -Wno-literal-suffix
            -Wno-class-memaccess
            -Wno-strict-aliasing
            -Wno-maybe-uninitialized
            -Wno-stringop-truncation
    )

    target_compile_options(${ProjectName}
            PRIVATE
            $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:${MSCV_FLAGS}>
            $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:${CLANG_GCC_FLAGS}>
            $<$<OR:$<CXX_COMPILER_ID:Clang>>:${CLANG_FLAGS}>
            $<$<OR:$<CXX_COMPILER_ID:GNU>>:${GCC_FLAGS}>
            $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/bigobj>  # big object files
    )
endfunction(SetCompilerFlags)

function(SetDefaultCompileDefinitions ProjectName)
    if (NOT TARGET ${ProjectName})
        message(AUTHOR_WARNING "${ProjectName} is not a target, thus no compiler warning were added.")
        return()
    endif ()

    target_compile_definitions(${ProjectName}
            PUBLIC
            $<$<CONFIG:Release>:NDEBUG>
            $<$<CONFIG:Debug>:_DEBUG>
            # Windows
            $<$<PLATFORM_ID:Windows>:NOMINMAX>
            $<$<PLATFORM_ID:Windows>:UNICODE>
            # MSVC C++ 
            $<$<CXX_COMPILER_ID:MSVC>:_USE_MATH_DEFINES>
            $<$<CXX_COMPILER_ID:MSVC>:_SCL_SECURE_NO_WARNINGS>
            $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
            $<$<CXX_COMPILER_ID:MSVC>:_ENABLE_EXTENDED_ALIGNED_STORAGE>
            $<$<CXX_COMPILER_ID:MSVC>:_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING>
            PRIVATE
            # Clang.
            $<$<CXX_COMPILER_ID:Clang>:_MSC_EXTENSIONS> # enable MS extensions
            #            $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
    )

    if (MSVC)
        target_compile_options(${ProjectName} PUBLIC /utf-8)
    else ()
        target_compile_options(${ProjectName} PUBLIC
                -finput-charset=UTF-8
                -fexec-charset=UTF-8
        )
    endif ()
endfunction(SetDefaultCompileDefinitions)

function(AssignSourceGroup)
    foreach (_source IN ITEMS ${ARGN})
        if (IS_ABSOLUTE "${_source}")
            file(RELATIVE_PATH _source_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_source}")
        else ()
            set(_source_rel "${_source}")
        endif ()
        get_filename_component(_source_path "${_source_rel}" PATH)
        string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
        source_group("${_source_path_msvc}" FILES "${_source}")
    endforeach ()
endfunction(AssignSourceGroup)

# ==============================================================================
# AddTestProgram：添加单元测试程序
# ==============================================================================

macro(AddTestProgram TestFile Libraries Category)
    get_filename_component(FILE_NAME ${TestFile} NAME_WE)
    add_executable(${FILE_NAME} ${TestFile})

    set_target_properties(${FILE_NAME} PROPERTIES FOLDER "Tests/${Category}")

    target_link_libraries(${FILE_NAME} PUBLIC ${Libraries})
    set_target_properties(${FILE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${BEE_BINARY_DIR})
    add_test(NAME "${FILE_NAME}Test" COMMAND ${FILE_NAME} WORKING_DIRECTORY ${BEE_BINARY_DIR})
endmacro()

# ==============================================================================
# AddBenchProgram：添加基准程序
# ==============================================================================

macro(AddBenchProgram BenchFile Libraries Category)
    get_filename_component(FILE_NAME ${BenchFile} NAME_WE)
    add_executable(${FILE_NAME} ${BenchFile})

    set_target_properties(${FILE_NAME} PROPERTIES FOLDER "Benchmarks/${Category}")

    SetDefaultCompileDefinitions(${FILE_NAME})
    target_link_libraries(${FILE_NAME} PRIVATE ${Libraries})
    set_target_properties(${FILE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${BEE_BINARY_DIR})
endmacro()

# ==============================================================================
# 预编译头配置函数：BeeConfigurePrecompiledHeaders
# ==============================================================================

function(BeeConfigurePrecompiledHeaders)
    set(options "")
    set(oneValueArgs TARGET_NAME)
    set(multiValueArgs POTENTIAL_HEADERS)

    cmake_parse_arguments(PCH "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT TARGET ${PCH_TARGET_NAME})
        message(AUTHOR_WARNING "${PCH_TARGET_NAME} is not a target")
        return()
    endif ()

    # 标准库头文件
    set(STD_HEADERS
            <memory>
            <vector>
            <string>
            <unordered_map>
            <unordered_set>
            <map>
            <set>
            <array>
            <algorithm>
            <functional>
            <type_traits>
            <utility>
            <chrono>
            <thread>
            <mutex>
            <atomic>
            <future>
            <iostream>
            <fstream>
            <sstream>
            <cassert>
            <cstdint>
            <cstddef>
            <cmath>
    )

    # Bee 的核心头文件
    set(BEE_HEADERS
            "${CMAKE_SOURCE_DIR}/Source/BeeLib/Core/Base/Defines.hpp"
            "${CMAKE_SOURCE_DIR}/Source/BeeLib/Core/Base/Portable.hpp"
            "${CMAKE_SOURCE_DIR}/Source/BeeLib/Core/Logger/Logger.hpp"
            "${CMAKE_SOURCE_DIR}/Source/BeeLib/Core/Error/Exception.hpp"
            "${CMAKE_SOURCE_DIR}/Source/BeeLib/Core/Error/Error.hpp"
            "${CMAKE_SOURCE_DIR}/Source/BeeLib/Core/Error/Guardian.hpp"
    )

    foreach (header ${PCH_POTENTIAL_HEADERS})
        if (EXISTS "${header}")
            list(APPEND BEE_HEADERS "${header}")
        endif ()
    endforeach ()

    # 应用预编译头
    if (STD_HEADERS)
        target_precompile_headers(${PCH_TARGET_NAME} PRIVATE ${STD_HEADERS})

        if (BEE_HEADERS)
            target_precompile_headers(${PCH_TARGET_NAME} PRIVATE ${BEE_HEADERS})
        endif ()
    endif ()
endfunction()

# ============================================================================
# BeeAddLayer 函数实现
# ============================================================================

function(BeeAddLayer)
    set(options "")
    set(oneValueArgs NAME TYPE)
    set(multiValueArgs SOURCES DEPENDENCIES COMPONENTS COMPILE_DEFINITIONS COMPILE_OPTIONS INCLUDE_DIRECTORIES)

    cmake_parse_arguments(LAYER "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT LAYER_NAME)
        message(FATAL_ERROR "BeeAddLayer: NAME is required")
    endif ()

    # 默认静态库
    if (NOT LAYER_TYPE)
        set(LAYER_TYPE "STATIC")
    endif ()

    if (LAYER_TYPE MATCHES "STATIC")
        if (NOT LAYER_SOURCES)
            message(FATAL_ERROR "Static layer target have need source files.")
        endif ()
    endif ()

    # 创建目标名称
    set(target_name "Bee${LAYER_NAME}")

    # 检查目标是否已存在
    if (TARGET ${target_name})
        message(WARNING "Target ${target_name} already exists, skipping creation")
        return()
    endif ()

    # 创建层级库
    if (LAYER_TYPE STREQUAL "STATIC")
        # 创建静态库
        add_library(${target_name} STATIC ${LAYER_SOURCES})
    else ()
        # 创建接口库
        add_library(${target_name} INTERFACE)
    endif ()

    if (LAYER_TYPE STREQUAL "STATIC")
        set_target_properties(${target_name} PROPERTIES
                CXX_STANDARD 23
                CXX_STANDARD_REQUIRED ON
                CXX_EXTENSIONS OFF
        )
    else ()
        target_compile_features(${target_name} INTERFACE cxx_std_23)
    endif ()

    # 设置包含目录
    set(include_scope PUBLIC)
    if (LAYER_TYPE STREQUAL "INTERFACE")
        set(include_scope INTERFACE)
    endif ()

    # 默认包含目录
    target_include_directories(${target_name} ${include_scope}
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    )

    # 包含目录
    if (LAYER_INCLUDE_DIRECTORIES)
        target_include_directories(${target_name} ${include_scope} ${LAYER_INCLUDE_DIRECTORIES})
    endif ()

    # 处理依赖关系
    if (LAYER_DEPENDENCIES)
        target_link_libraries(${target_name} ${include_scope} ${LAYER_DEPENDENCIES})
    endif ()

    # 处理编译定义
    if (LAYER_COMPILE_DEFINITIONS)
        target_compile_definitions(${target_name} ${include_scope} ${LAYER_COMPILE_DEFINITIONS})
    endif ()

    # 处理编译选项
    if (LAYER_COMPILE_OPTIONS)
        target_compile_options(${target_name} ${include_scope} ${LAYER_COMPILE_OPTIONS})
    endif ()

    # 创建别名
    add_library(Bee::${LAYER_NAME} ALIAS ${target_name})

    # 设置导出名称
    set_target_properties(${target_name} PROPERTIES
            EXPORT_NAME ${LAYER_NAME}
            OUTPUT_NAME "Bee${LAYER_NAME}"
            RUNTIME_OUTPUT_DIRECTORY ${BEE_BINARY_DIR}
    )

    # 链接组件
    if (LAYER_COMPONENTS)
        foreach (component ${LAYER_COMPONENTS})
            if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${component}/CMakeLists.txt")
                add_subdirectory(${component})
#                message(STATUS "  Added component: ${component}")
            else ()
                message(WARNING "  Component directory not found: ${component}")
            endif ()

            set(component_target "Bee${LAYER_NAME}_${component}")
            if (TARGET ${component_target})
                target_link_libraries(${target_name} ${include_scope} ${component_target})
                message(STATUS "  Added component: ${component}")
            else ()
                message(WARNING "  Component ${component} not available")
            endif ()
        endforeach ()
    endif ()

#    message(STATUS "Layer ${LAYER_NAME} created successfully as ${target_name}")
endfunction()

# ============================================================================
# BeeAddComponent 函数实现
# ============================================================================

function(BeeAddComponent)
    set(options "")
    set(oneValueArgs NAME LAYER)
    set(multiValueArgs SOURCES HEADERS COMPILE_DEFINITIONS COMPILE_OPTIONS INCLUDE_DIRECTORIES LINK_LIBRARIES)

    cmake_parse_arguments(COMP "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT COMP_NAME)
        message(FATAL_ERROR "BeeAddComponent: NAME is required")
    endif ()

    if (NOT COMP_LAYER)
        message(FATAL_ERROR "BeeAddComponent: LAYER is required")
    endif ()

    # 创建目标名称
    set(target_name "Bee${COMP_LAYER}_${COMP_NAME}")
    set(layer_target "Bee${COMP_LAYER}")

    # 检查目标是否已存在
    if (TARGET ${target_name})
        message(WARNING "Target ${target_name} already exists, skipping creation")
        return()
    endif ()

    # 确定是否为仅头文件库
    set(is_header_only FALSE)
    if (NOT COMP_SOURCES)
        set(is_header_only TRUE)
        message(STATUS "  Detected as header-only component")
    endif ()

    # 创建组件库
    if (is_header_only)
        # 创建接口库
        add_library(${target_name} INTERFACE)
        set(include_scope INTERFACE)
    else ()
        # 创建静态库
        add_library(${target_name} STATIC ${COMP_SOURCES})
        set(include_scope PUBLIC)

        # 设置C++标准
        set_target_properties(${target_name} PROPERTIES
                CXX_STANDARD 23
                CXX_STANDARD_REQUIRED ON
                CXX_EXTENSIONS OFF
        )
        
        # 设置编译器标志
        SetCompilerFlags(${target_name})
    endif ()


    # 默认包含目录
    target_include_directories(${target_name} ${include_scope}
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            ${BEE_LIB_DIR}
    )

    # 用户指定的包含目录
    if (COMP_INCLUDE_DIRECTORIES)
        target_include_directories(${target_name} ${include_scope} ${COMP_INCLUDE_DIRECTORIES})
    endif ()

    # 处理编译定义
    if (COMP_COMPILE_DEFINITIONS)
        target_compile_definitions(${target_name} ${include_scope} ${COMP_COMPILE_DEFINITIONS})
    endif ()

    # 处理编译选项
    if (COMP_COMPILE_OPTIONS)
        target_compile_options(${target_name} ${include_scope} ${COMP_COMPILE_OPTIONS})
    endif ()

    # 处理链接库
    if (COMP_LINK_LIBRARIES)
        target_link_libraries(${target_name} ${include_scope} ${COMP_LINK_LIBRARIES})
    endif ()

    # 创建别名
    add_library(Bee::${COMP_LAYER}::${COMP_NAME} ALIAS ${target_name})

    # 设置导出名称
    set_target_properties(${target_name} PROPERTIES
            EXPORT_NAME "${COMP_LAYER}_${COMP_NAME}"
            OUTPUT_NAME "Bee${COMP_LAYER}_${COMP_NAME}"
            RUNTIME_OUTPUT_DIRECTORY ${BEE_BINARY_DIR}
    )

#    message(STATUS "Component ${COMP_LAYER}::${COMP_NAME} created successfully as ${target_name}")
endfunction()