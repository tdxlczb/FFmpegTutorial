cmake_minimum_required(VERSION 3.24)

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
