# QT
set(Qt_DIR $ENV{Qt6_DIR})
list(APPEND CMAKE_PREFIX_PATH ${Qt_DIR})

if (WIN32)
endif (WIN32)

function(BeeDeployQT ProjectName)
    if (NOT TARGET ${ProjectName})
        message(AUTHOR_WARNING "${ProjectName} is not a target, thus no compiler warning were added.")
        return()
    endif ()
    
    if (WIN32)
     add_custom_command(TARGET ${ProjectName} 
             POST_BUILD COMMAND
            "${Qt_DIR}/bin/windeployqt.exe" ${${PROJECT_NAME_UPPERCASE}_BINARY_DIR}/${BEE_TARGET}.exe --verbose 0
     )
    endif (WIN32)
    
endfunction(BeeDeployQT)