cmake_minimum_required(VERSION 3.24)

project(FFmpegTutorial
    VERSION 0.0.1
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/MP) # 并行编译

    add_compile_options(/utf-8)
    add_compile_options(/Zc:__cplusplus)

    #add_compile_options(/W4) # 新项目推荐使用的警告等级

    add_compile_options(/Zi) # pdb 格式

    # 额外优化
    add_compile_options("$<$<NOT:$<CONFIG:Debug>>:/Oi>") # Intrinsic Function
    add_compile_options("$<$<NOT:$<CONFIG:Debug>>:/Gy>") # function-level linkage

    # 更符合标准
    add_compile_options("/permissive-")

    add_compile_definitions(UNICODE _UNICODE) # Unicode

    #add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    #add_compile_definitions(NOGDI)
    #add_compile_definitions(_HAS_EXCEPTIONS=0)

    # 为非 Debug 也生成 pdb
    add_link_options("$<$<NOT:$<CONFIG:Debug>>:/DEBUG>")
    add_link_options("$<$<NOT:$<CONFIG:Debug>>:/OPT:REF>")
    add_link_options("$<$<NOT:$<CONFIG:Debug>>:/OPT:ICF>")
endif()

include(cmake/install_helper.cmake)
include(cmake/nuget.cmake)

set(FFMPEG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg/ffmpeg-4.2.1/dynamic)

add_subdirectory(tutorial)

#设置开启使用目录
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)
#添加目录
#SET_PROPERTY(TARGET tutorial PROPERTY FOLDER "Tutorial")

