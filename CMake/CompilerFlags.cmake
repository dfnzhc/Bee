function(set_compiler_flags project_name)
    if (NOT TARGET ${project_name})
        message(AUTHOR_WARNING "${project_name} is not a target, thus no compiler warning were added.")
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
            #            -fvisibility=hidden             # hide symbols by default
            -W
            -Wall                           # set warning level
            -Wno-unused-function
            -Wno-unused-variable
            -Wno-unused-but-set-variable
            -Wno-switch
            -Wno-missing-braces
            -Wno-invalid-offsetof
    )

    set(CLANG_FLAGS
            -Wno-unused-private-field
            -Wno-braced-scalar-init
            -Wno-self-assign-overloaded
            -Wno-c++98-compat
            -Wno-c++98-compat-pedantic
            -Wno-newline-eof
            -Wno-global-constructors
            -Wno-ctad-maybe-unsupported
            -Wno-undef
            -Wno-exit-time-destructors
            -Wno-gnu-anonymous-struct
            -Wno-nested-anon-types
            -Wno-old-style-cast
            -Wno-double-promotion
            -Wno-float-equal
            -Wno-c++20-compat
            -Wno-unsafe-buffer-usage
            -Wno-cast-function-type-strict
            -Wno-documentation-unknown-command
            -Wno-documentation
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

    target_compile_options(${project_name}
            PUBLIC
            $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:${MSCV_FLAGS}>
            $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:${CLANG_GCC_FLAGS}>
            $<$<OR:$<CXX_COMPILER_ID:Clang>>:${CLANG_FLAGS}>
            $<$<OR:$<CXX_COMPILER_ID:GNU>>:${GCC_FLAGS}>
            PRIVATE
            $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/bigobj>  # big object files
    )
endfunction(set_compiler_flags)