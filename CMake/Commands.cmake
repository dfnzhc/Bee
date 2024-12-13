# Copy data files to output directory
#add_custom_target(CopyDataFolder ALL
#        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/Data ${${PROJECT_NAME_UPPERCASE}_OUTPUT_DIR}/Data)
#set_target_properties(CopyDataFolder PROPERTIES FOLDER "Misc")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/.clang-format ${CMAKE_CURRENT_BINARY_DIR}/.clang-format COPYONLY)