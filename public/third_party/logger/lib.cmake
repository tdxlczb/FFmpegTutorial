include(${PUBLIC_CMAKE_DIR}/nuget.cmake)

macro(INSTALL_LOGGER TARGET_NAME)
if(WIN32)

    if(CMAKE_CL_64) # CMAKE的内建变量，如果是true，就说明编译器的64位的，自然可以编译64bit的程序
        set(PLATFORM x64)
    else()
        set(PLATFORM win32)
    endif()

    set(LOGGER_INC ${THIRD_PARTY_DIR}/logger/include)
    set(LOGGER_LIB ${THIRD_PARTY_DIR}/logger/bin/${PLATFORM}/$<IF:$<CONFIG:Debug>,debug,release>)
    set(LOGGER_BIN ${THIRD_PARTY_DIR}/logger/bin/${PLATFORM}/$<IF:$<CONFIG:Debug>,debug,release>)
    
    target_include_directories(${TARGET_NAME} PRIVATE ${LOGGER_INC})
    target_link_directories(${TARGET_NAME} PRIVATE ${LOGGER_LIB})
    target_link_libraries(${TARGET_NAME} PRIVATE glog)

    # file(GLOB LIB_FILES "${LOGGER_BIN}/*.dll") # 生成器表达式在生成构建环境阶段生效，这里file文件操作并不能获取到正确路径
    get_target_property(OUTPUT_DIR ${TARGET_NAME} RUNTIME_OUTPUT_DIRECTORY)
    # install(FILES ${LIB_FILES} DESTINATION ${OUTPUT_DIR}) # 安装到输出目录
    # install(FILES ${LOGGER_BIN}/glog.dll DESTINATION ${OUTPUT_DIR}) # 安装到输出目录，这里install的时候生成器表达式可以生效
    # install(FILES ${LIB_FILES} DESTINATION $<CONFIG>) # 安装到install-prefix指定的目录
    # install(DIRECTORY ${LOGGER_BIN}/ DESTINATION ${OUTPUT_DIR})
    install(DIRECTORY ${LOGGER_BIN}/ DESTINATION ${OUTPUT_DIR} FILES_MATCHING PATTERN "*.dll")

    # 生成后事件也支持生成器表达式
    # add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    #     COMMAND ${CMAKE_COMMAND} -E copy_directory ${LOGGER_BIN}/ ${CURRENT_OUTPUT_DIRECTORY}/
    # )
endif()
endmacro()