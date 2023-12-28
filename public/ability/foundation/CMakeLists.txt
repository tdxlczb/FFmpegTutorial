set(TARGET_NAME foundation)

file(GLOB_RECURSE ${TARGET_NAME}_SOURCES FOLLOW_SYMLINKS LIST_DIRECTORIES false CONFIGURE_DEPENDS
     *.h
     *.cpp
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${${TARGET_NAME}_SOURCES})

add_library(${TARGET_NAME} STATIC)
target_sources(${TARGET_NAME} PRIVATE ${${TARGET_NAME}_SOURCES})
# 将模块添加到自定义目录中
set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER "ability")
set_target_properties(${TARGET_NAME} PROPERTIES OUTPUT_NAME ${TARGET_NAME}$<$<CONFIG:Debug>:d>)

target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

