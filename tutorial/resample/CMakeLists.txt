set(TARGET_NAME resample)
add_executable(${TARGET_NAME})

file(GLOB_RECURSE ${TARGET_NAME}_SOURCES FOLLOW_SYMLINKS LIST_DIRECTORIES false CONFIGURE_DEPENDS
     *.h
     *.cpp
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${${TARGET_NAME}_SOURCES})
target_sources(${TARGET_NAME} PRIVATE ${${TARGET_NAME}_SOURCES})

include_directories(${FFMPEG_DIR}/include)

target_link_directories(${TARGET_NAME} PRIVATE ${FFMPEG_DIR}/lib/x86)

#target_link_libraries(${TARGET_NAME} PRIVATE avcodec$<$<CONFIG:Debug>:d>)
#target_link_libraries(${TARGET_NAME} PRIVATE avdevice$<$<CONFIG:Debug>:d>)
#target_link_libraries(${TARGET_NAME} PRIVATE avfilter$<$<CONFIG:Debug>:d>)
#target_link_libraries(${TARGET_NAME} PRIVATE avformat$<$<CONFIG:Debug>:d>)
target_link_libraries(${TARGET_NAME} PRIVATE avutil$<$<CONFIG:Debug>:d>)
#target_link_libraries(${TARGET_NAME} PRIVATE postproc$<$<CONFIG:Debug>:d>)
target_link_libraries(${TARGET_NAME} PRIVATE swresample$<$<CONFIG:Debug>:d>)
#target_link_libraries(${TARGET_NAME} PRIVATE swscale$<$<CONFIG:Debug>:d>)

install_executable(${TARGET_NAME})