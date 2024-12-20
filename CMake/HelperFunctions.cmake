# 设置项目的编译器标志
function(SetCompilerFlags ProjectName)
    if (NOT TARGET ${ProjectName})
        message(AUTHOR_WARNING "${ProjectName} is not a target, thus no compiler warning were added.")
        return()
    endif ()

    set(MSVC_FLAGS
            /Zi                             # generate debug symbols
            /WX                             # warnings as errors
            /W4                             # increase warning level
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
            /MP                             # enable multi-processor compilation
    )

    set(CLANG_GCC_FLAGS
            -fms-extensions                 # enable MS extensions (among other things allow anonymous structs)
            -Wall                           # set warning level
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
            PUBLIC
            $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:${MSCV_FLAGS}>
            $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:${CLANG_GCC_FLAGS}>
            $<$<OR:$<CXX_COMPILER_ID:Clang>>:${CLANG_FLAGS}>
            $<$<OR:$<CXX_COMPILER_ID:GNU>>:${GCC_FLAGS}>
            PRIVATE
            $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/bigobj>  # big object files
    )
endfunction(SetCompilerFlags)


# 设置项目定义
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
            # Clang.
            $<$<CXX_COMPILER_ID:Clang>:_MSC_EXTENSIONS> # enable MS extensions
            PRIVATE
            $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
            ${PROJECT_NAME_UPPERCASE}_DLL
    )
endfunction(SetDefaultCompileDefinitions)

# 项目的默认定义
function(BeeDefaultSettings ProjectName)
    if (NOT TARGET ${ProjectName})
        message(AUTHOR_WARNING "${ProjectName} is not a target, thus no compiler warning were added.")
        return()
    endif ()
    set(ProjectBriefName ${ProjectName})
    string(REPLACE "Bee" "" ProjectBriefName "${ProjectBriefName}")

    SetCompilerFlags(${ProjectName})
    SetDefaultCompileDefinitions(${ProjectName})

    target_compile_features(${ProjectName} PUBLIC cxx_std_23)

    add_custom_command(TARGET ${ProjectName} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE_DIR:${ProjectName}>/${ProjectName}.dll"
            ${${PROJECT_NAME_UPPERCASE}_BINARY_DIR})

    # TODO: 临时的解决方法
    add_custom_command(TARGET ${ProjectName} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE_DIR:${ProjectName}>/${ProjectName}.dll"
            ${CMAKE_BINARY_DIR}/Tests)

    add_library(Bee::${ProjectBriefName} ALIAS ${ProjectName})
endfunction(BeeDefaultSettings)


# 设置代码文件的 group
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

# 添加测试程序
function(AddTestProgram TestFile Libraries)
    get_filename_component(FILE_NAME ${TestFile} NAME_WE)
    add_executable(${FILE_NAME} ${TestFile})
    set_target_properties(${FILE_NAME}
            PROPERTIES
            FOLDER "Tests")

    foreach (Lib ${Libraries})
        target_link_libraries(${FILE_NAME}
                PUBLIC ${Lib})
    endforeach ()

    add_test(NAME "${FILE_NAME}Test"
            COMMAND ${FILE_NAME}
            WORKING_DIRECTORY ${${PROJECT_NAME_UPPERCASE}_BINARY_DIR})

endfunction(AddTestProgram)
