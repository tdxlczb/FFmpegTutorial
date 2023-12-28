include(${PUBLIC_CMAKE_DIR}/nuget.cmake)

macro(INSTALL_FFMPEG TARGET_NAME)
if(WIN32)
    set(FFMPEG_DIR ffmpeg-5.1.4)
    
    set(FFMPEG_INC ${THIRD_PARTY_DIR}/ffmpeg/${FFMPEG_DIR}/include)
    set(FFMPEG_LIB ${THIRD_PARTY_DIR}/ffmpeg/${FFMPEG_DIR}/lib)
    set(FFMPEG_BIN ${THIRD_PARTY_DIR}/ffmpeg/${FFMPEG_DIR}/bin)
    target_include_directories(${TARGET_NAME} PRIVATE ${FFMPEG_INC})
    target_link_directories(${TARGET_NAME} PRIVATE ${FFMPEG_LIB})

    target_link_libraries(${TARGET_NAME} PRIVATE avcodec$<$<CONFIG:Debug>:d>)
    target_link_libraries(${TARGET_NAME} PRIVATE avdevice$<$<CONFIG:Debug>:d>)
    target_link_libraries(${TARGET_NAME} PRIVATE avfilter$<$<CONFIG:Debug>:d>)
    target_link_libraries(${TARGET_NAME} PRIVATE avformat$<$<CONFIG:Debug>:d>)
    target_link_libraries(${TARGET_NAME} PRIVATE avutil$<$<CONFIG:Debug>:d>)
    target_link_libraries(${TARGET_NAME} PRIVATE postproc$<$<CONFIG:Debug>:d>)
    target_link_libraries(${TARGET_NAME} PRIVATE swresample$<$<CONFIG:Debug>:d>)
    target_link_libraries(${TARGET_NAME} PRIVATE swscale$<$<CONFIG:Debug>:d>)

    install(DIRECTORY ${FFMPEG_BIN}/ DESTINATION ${CURRENT_OUTPUT_DIRECTORY})
endif()
endmacro()