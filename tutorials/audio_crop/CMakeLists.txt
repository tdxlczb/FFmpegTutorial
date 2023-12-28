include(${THIRD_PARTY_DIR}/third_party.cmake)

set(TARGET_NAME audio_crop)

file(GLOB_RECURSE ${TARGET_NAME}_SOURCES FOLLOW_SYMLINKS LIST_DIRECTORIES false CONFIGURE_DEPENDS
     *.h
     *.cpp
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${${TARGET_NAME}_SOURCES})

add_executable(${TARGET_NAME})
target_sources(${TARGET_NAME} PRIVATE ${${TARGET_NAME}_SOURCES})
# 将模块添加到自定义目录中
set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER "tutorials")

target_link_libraries(${TARGET_NAME} PRIVATE foundation logger)

INSTALL_FFMPEG(${TARGET_NAME})

# install_executable(${TARGET_NAME})
# install(TARGETS ${TARGET_NAME} DESTINATION bin)


