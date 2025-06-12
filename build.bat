@echo off

set "ROOT_DIR=%~dp0"

:: clear temp dir
@REM rd /q /s build
@REM rd /q /s output

:: build
@REM cmake -S . -B build -G "Visual Studio 14 2015" -A Win32 -T v140
cmake -S . -B build -G "Visual Studio 14 2015" -A x64 -T v140
@REM cmake -S . -B build -A Win32
@REM cmake -S . -B build -A x64
@REM cmake -S . -B build -A Win32 -DCMAKE_INSTALL_PREFIX="%ROOT_DIR%\build"
@REM cmake -S . -B build -A x64 -DCMAKE_INSTALL_PREFIX="%ROOT_DIR%\build"

@REM cmake --build build --config debug
@REM cmake --build build --config release
@REM cmake -P build\cmake_install.cmake

@REM cmake --install build --prefix output --config debug
cmake --install build --prefix output --config release 


@echo on

