@echo off

set "ROOT_DIR=%~dp0"

:: clear temp dir
rd /q /s build
rd /q /s output

:: build
@REM cmake -S . -B build -A Win32
@REM cmake -S . -B build -A x64  -DUSE_FFMPEG_5_1=ON  -DCMAKE_INSTALL_PREFIX="%ROOT_DIR%\build"
cmake -S . -B build -A x64  -DUSE_FFMPEG_5_1=ON

@REM cmake --build build --config debug
@REM cmake --build build --config release
@REM cmake -P build\cmake_install.cmake

@REM cmake --install build --prefix output --config debug
cmake --install build --prefix output --config release 


@echo on

