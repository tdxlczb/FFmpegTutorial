cmake_minimum_required(VERSION 3.24)

macro(install_with_directory)
    set(optionsArgs "")
    set(oneValueArgs "DESTINATION")
    set(multiValueArgs "FILES")
    cmake_parse_arguments(CAS "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    foreach(FILE ${CAS_FILES})
        get_filename_component(DIR ${FILE} DIRECTORY)
        install(FILES ${FILE} DESTINATION $<PATH:APPEND,${CAS_DESTINATION},${DIR}>)
    endforeach()
endmacro(install_with_directory)

macro(install_executable TARGET_NAME)
  install(TARGETS ${TARGET_NAME}
          RUNTIME DESTINATION $<PATH:APPEND,$<CONFIG>,bin>
          ARCHIVE DESTINATION $<PATH:APPEND,$<CONFIG>,lib>
  )
  install(FILES $<TARGET_PDB_FILE:${TARGET_NAME}> DESTINATION $<PATH:APPEND,$<CONFIG>,pdb>)
  # 复制运行时依赖
  install(FILES $<TARGET_RUNTIME_DLLS:${TARGET_NAME}> DESTINATION $<PATH:APPEND,$<CONFIG>,bin>)
  install(DIRECTORY $<TARGET_FILE_DIR:${TARGET_NAME}>/
          DESTINATION $<PATH:APPEND,$<CONFIG>,bin>
          PATTERN "*.exe" EXCLUDE
          PATTERN "*.pdb" EXCLUDE
          PATTERN "*.lib" EXCLUDE
          PATTERN "*.exp" EXCLUDE
  )
endmacro()

macro(install_shared_library TARGET_NAME)
  install(TARGETS ${TARGET_NAME}
          RUNTIME DESTINATION $<PATH:APPEND,$<CONFIG>,bin>
          ARCHIVE DESTINATION $<PATH:APPEND,$<CONFIG>,lib>
  )
  install(FILES $<TARGET_PDB_FILE:${TARGET_NAME}> DESTINATION $<PATH:APPEND,$<CONFIG>,pdb>)
  install_with_directory(FILES ${${TARGET_NAME}_INCLUDES} DESTINATION include)
endmacro()

macro(install_static_library TARGET_NAME)
  install(TARGETS ${TARGET_NAME}
          ARCHIVE DESTINATION $<PATH:APPEND,$<CONFIG>,lib>
  )
  install_with_directory(FILES ${${TARGET_NAME}_INCLUDES} DESTINATION include)
endmacro()
