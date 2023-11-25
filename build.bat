@echo off

:: clear temp dir
rd /q /s build
rd /q /s output

:: build
@REM cmake -S . -B build -A Win32
cmake -S . -B build -A x64 -DUSE_FFMPEG_5_1=ON

::cmake --build build --config debug
::cmake --build build --config release 

::cmake --install build --prefix output --config debug
::cmake --install build --prefix output --config release 


@echo on

