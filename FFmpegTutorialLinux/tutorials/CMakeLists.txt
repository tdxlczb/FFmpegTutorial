set(TARGET_NAME video_decode)

file(GLOB_RECURSE ${TARGET_NAME}_SOURCES FOLLOW_SYMLINKS LIST_DIRECTORIES false CONFIGURE_DEPENDS
     *.h
     *.cpp
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${${TARGET_NAME}_SOURCES})

add_executable(${TARGET_NAME})
target_sources(${TARGET_NAME} PRIVATE ${${TARGET_NAME}_SOURCES})
# 将模块添加到自定义目录中
set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER "tutorials")


if(WIN32)
else()
     if(USE_CUSTOM_FFMPEG)
          target_include_directories(${TARGET_NAME} PRIVATE "/home/cvte/ffmpeg-build/libs/include")
          target_link_directories(${TARGET_NAME} PRIVATE "/home/cvte/ffmpeg-build/libs/lib")
          target_link_libraries(${TARGET_NAME} PRIVATE avcodec avformat avutil swscale)
          add_definitions("-DUSE_SCALE")
     else()
          find_package(PkgConfig REQUIRED)
          pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libavutil)
          target_include_directories(${TARGET_NAME} PUBLIC ${FFMPEG_INCLUDE_DIRS})
          target_link_directories(${TARGET_NAME} PRIVATE ${FFMPEG_LIBRARY_DIRS})
          target_link_libraries(${TARGET_NAME} PRIVATE ${FFMPEG_LIBRARIES})
     endif()

     target_link_libraries(${TARGET_NAME} PRIVATE pthread)
endif()

