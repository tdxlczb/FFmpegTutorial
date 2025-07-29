cmake_minimum_required(VERSION 3.16)
# 这里是工程名称  C工程
project(fftools C)

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
file(GLOB ffmpeg_sources ${CMAKE_SOURCE_DIR}/*.*)
list(FILTER ffmpeg_sources EXCLUDE REGEX "ffplay.c")
list(FILTER ffmpeg_sources EXCLUDE REGEX "ffprobe.c")

add_executable(ffmpeg_debug ${ffmpeg_sources})

#链接库
target_link_libraries(ffmpeg_debug
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

file(GLOB ffplay_sources ${CMAKE_SOURCE_DIR}/*.*)
list(FILTER ffplay_sources EXCLUDE REGEX "ffmpeg.c")
list(FILTER ffplay_sources EXCLUDE REGEX "ffprobe.c")

add_executable(ffplay_debug ${ffplay_sources})
target_link_libraries(ffplay_debug
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

file(GLOB ffprobe_sources ${CMAKE_SOURCE_DIR}/*.*)
list(FILTER ffprobe_sources EXCLUDE REGEX "ffmpeg.c")
list(FILTER ffprobe_sources EXCLUDE REGEX "ffplay.c")

add_executable(ffprobe_debug ${ffprobe_sources})
target_link_libraries(ffprobe_debug
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