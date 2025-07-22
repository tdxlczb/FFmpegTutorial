cmake_minimum_required(VERSION 3.16)
# 这里是工程名称  C工程
project(FFplay_debug C)

# set(CMAKE_C_STANDARD 99)

set(THIRD_PARTY_DIR ${CMAKE_CURRENT_LIST_DIR}/../public/third_party)

# FFmpeg的安装目录，可以通过命令"brew info ffmpeg"获取
# 需要替换成自己的FFMpeg安装目录
set(FFMPEG_DIR ${THIRD_PARTY_DIR}/ffmpeg/ffmpeg_5.1)

# SDL2的安装目录，可以通过命令"brew info sdl2"获取
# 需要替换成自己的SDL2安装目录
set(SDL2_DIR ${THIRD_PARTY_DIR}/sdl/sdl_2.30)

message(STATUS "FFMPEG_DIR = ${FFMPEG_DIR}")

# 头文件搜索路径
include_directories(${FFMPEG_DIR}/include/)

include_directories(${CMAKE_SOURCE_DIR})

include_directories(${SDL2_DIR}/include/SDL2/)

# 动态链接库或静态链接库的搜索路径
link_directories(${FFMPEG_DIR}/lib/)

link_directories(${SDL2_DIR}/lib/)

#将指定文件设置在FFmpeg_test_source变量中
file(GLOB FFplay_debug_source ${CMAKE_SOURCE_DIR}/*.*)

add_executable(FFplay_debug ${FFplay_debug_source} cmdutils.c)

#链接库
target_link_libraries(FFplay_debug
        #FFmpeg 库
        avcodec
        avdevice
        postproc
        avfilter
        avformat
        avutil
        swresample
        swscale
        # SDL2库
        SDL2
        )