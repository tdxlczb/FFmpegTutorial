cmake_minimum_required(VERSION 3.16)

project(FFmpegTutorial
    VERSION 0.0.1
    LANGUAGES CXX
)

include(const.cmake)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<$<CONFIG:Release>:Release>$<$<CONFIG:Debug>:Debug>)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<$<CONFIG:Release>:Release>$<$<CONFIG:Debug>:Debug>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set(CURRENT_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# 生成器表达式在生成构建环境阶段生效，这里message打印不了生成器表达式的值
message(STATUS "output dir = ${CURRENT_OUTPUT_DIRECTORY}")

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
    add_compile_options("$<$<NOT:$<CONFIG:Debug>>:/Od>") # release 禁用优化

    # 更符合标准
    add_compile_options("/permissive-")

    add_compile_definitions(UNICODE _UNICODE) # Unicode

    # 为非 Debug 也生成 pdb
    add_link_options("$<$<NOT:$<CONFIG:Debug>>:/DEBUG>")
    add_link_options("$<$<NOT:$<CONFIG:Debug>>:/OPT:REF>")
    add_link_options("$<$<NOT:$<CONFIG:Debug>>:/OPT:ICF>")
endif()

#设置开启使用目录
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(public)
add_subdirectory(tutorials)

