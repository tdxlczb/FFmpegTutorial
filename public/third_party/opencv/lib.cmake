include(${PUBLIC_CMAKE_DIR}/nuget.cmake)

macro(INSTALL_OPENCV TARGET_NAME)
if(WIN32)
    set(OPENCV_DIR opencv-3.4.16)
    
    set(OPENCV_INC ${THIRD_PARTY_DIR}/opencv/${OPENCV_DIR}/opencv/build/include)
    set(OPENCV_LIB ${THIRD_PARTY_DIR}/opencv/${OPENCV_DIR}/opencv/build/x64/vc14/lib)
    set(OPENCV_BIN ${THIRD_PARTY_DIR}/opencv/${OPENCV_DIR}/opencv/build/x64/vc14/bin)
    target_include_directories(${TARGET_NAME} PRIVATE ${OPENCV_INC})
    target_link_directories(${TARGET_NAME} PRIVATE ${OPENCV_LIB})

    target_link_libraries(${TARGET_NAME} PRIVATE opencv_world3416$<$<CONFIG:Debug>:d>)

    file(GLOB LIB_FILES 
        "${OPENCV_BIN}/opencv_ffmpeg3416_64.dll"
        "${OPENCV_BIN}/opencv_world3416.dll"
    )
    get_target_property(OUTPUT_DIR ${TARGET_NAME} RUNTIME_OUTPUT_DIRECTORY)
    install(FILES ${LIB_FILES} DESTINATION ${OUTPUT_DIR}) # 安装到输出目录
    # install(FILES ${LIB_FILES} DESTINATION $<CONFIG>) # 安装到install-prefix指定的目录
    # install(DIRECTORY ${FFMPEG_BIN}/ DESTINATION ${OUTPUT_DIR})
endif()
endmacro()