set(SLANG_SDK_PATH "$ENV{SLANG_SDK_PATH}") 
if(NOT EXISTS ${SLANG_SDK_PATH} )
    message(FATAL_ERROR "Slang SDK not found.")
endif()

mark_as_advanced(SLANG_SDK_PATH)
set(SLANG_SDK_PATH ${SLANG_SDK_PATH} CACHE FILEPATH "Path to Slang SDK")

# slangc
find_program(SLANG_slangc
    NAMES slangc
    HINTS ${SLANG_SDK_PATH}/bin NO_DEFAULT_PATH
)

if(NOT EXISTS ${SLANG_slangc} )
    message(FATAL_ERROR "Slang compiler not found.")
endif()

set(SLANG_slangc ${SLANG_slangc} CACHE FILEPATH "Path to Slang compiler")

# slang dirs

set(SLANG_SDK_BIN_DIR ${SLANG_SDK_PATH}/bin)
set(SLANG_SDK_INCLUDES_DIR ${SLANG_SDK_PATH}/include)
set(SLANG_SDK_LIB_DIR ${SLANG_SDK_PATH}/lib)

mark_as_advanced(SLANG_SDK_BIN_DIR)
mark_as_advanced(SLANG_SDK_INCLUDES_DIR)
mark_as_advanced(SLANG_SDK_LIB_DIR)

set(SLANG_SDK_BIN_DIR ${SLANG_SDK_BIN_DIR} CACHE FILEPATH "Path to Slang binaries")
set(SLANG_SDK_INCLUDES_DIR ${SLANG_SDK_INCLUDES_DIR} CACHE FILEPATH "Path to Slang includes")
set(SLANG_SDK_LIB_DIR ${SLANG_SDK_LIB_DIR} CACHE FILEPATH "Path to Slang libraries")

execute_process(COMMAND ${SLANG_slangc} "-v" ERROR_VARIABLE SLANG_VERSION RESULTS_VARIABLE EXTRACT_RESULT)
if(NOT EXTRACT_RESULT EQUAL 0)
  message("Envoking Slang compiler failed with error code: ${EXTRACT_RESULT}")
endif()

message(STATUS "--> Using Slang SDK under: ${SLANG_SDK_PATH}, version: ${SLANG_VERSION}.")