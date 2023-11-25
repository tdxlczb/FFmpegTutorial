cmake_minimum_required(VERSION 3.16)

project(FFmpegTutorial
    VERSION 0.0.1
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/install_helper.cmake)
include(cmake/nuget.cmake)

if(MSVC)
    add_compile_options(/MP) # 并行编译

    add_compile_options(/utf-8)
    add_compile_options(/Zc:__cplusplus)

    add_compile_options(/W4) # 新项目推荐使用的警告等级

    add_compile_options(/Zi) # pdb 格式
    
    # 额外优化
    # 从 VS 默认的项目的配置中抠出来的
    add_compile_options("$<$<NOT:$<CONFIG:Debug>>:/Oi>") # Intrinsic Function
    add_compile_options("$<$<NOT:$<CONFIG:Debug>>:/Gy>") # function-level linkage

    # 更符合标准
    add_compile_options("/permissive-")

    add_compile_definitions(UNICODE _UNICODE) # Unicode

    # 为非 Debug 也生成 pdb
    add_link_options("$<$<NOT:$<CONFIG:Debug>>:/DEBUG>")
    add_link_options("$<$<NOT:$<CONFIG:Debug>>:/OPT:REF>")
    add_link_options("$<$<NOT:$<CONFIG:Debug>>:/OPT:ICF>")
endif()

set(FFMPEG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg)
if(USE_FFMPEG_4_2)
    add_definitions("-DUSE_FFMPEG_4_2")
    set(FFMPEG_INC ${FFMPEG_DIR}/ffmpeg-4.2.1/dynamic/include)
    set(FFMPEG_LIB ${FFMPEG_DIR}/ffmpeg-4.2.1/dynamic/lib/x86)
    set(FFMPEG_BIN ${FFMPEG_DIR}/ffmpeg-4.2.1/dynamic/bin/x86)
elseif(USE_FFMPEG_5_1)
    add_definitions("-DUSE_FFMPEG_5_1")
    set(FFMPEG_INC ${FFMPEG_DIR}/ffmpeg-5.1.4/include)
    set(FFMPEG_LIB ${FFMPEG_DIR}/ffmpeg-5.1.4/lib)
    set(FFMPEG_BIN ${FFMPEG_DIR}/ffmpeg-5.1.4/bin)
endif()

MESSAGE(STATUS "FFMPEG_INC = ${FFMPEG_INC}")
MESSAGE(STATUS "FFMPEG_LIB = ${FFMPEG_LIB}")
MESSAGE(STATUS "FFMPEG_BIN = ${FFMPEG_BIN}")

set(TOOLS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tools)
set(TUTORIALS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tutorials)

add_subdirectory(tools)
add_subdirectory(tutorials)

#设置开启使用目录
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)
#添加目录
#SET_PROPERTY(TARGET tutorials PROPERTY FOLDER "tutorials")

