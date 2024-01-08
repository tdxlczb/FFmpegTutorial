#lib.cmake封装了对应库的信息，便于跨平台以及子模块嵌套

macro(INCLUDE_LIB dirname)
    include(${CMAKE_CURRENT_LIST_DIR}/${dirname}/lib.cmake)
endmacro()

INCLUDE_LIB(ffmpeg)
INCLUDE_LIB(logger)
INCLUDE_LIB(opencv)
