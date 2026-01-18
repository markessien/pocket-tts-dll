@echo off
REM Build script for the C++ test program using MSVC

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

if not exist build mkdir build

cl /EHsc /Zi /wd4996 main.cpp /Fe:build\cpp_test.exe

if %errorlevel% equ 0 (
    echo Build successful.
    echo Copying DLL to build directory...
    copy ..\..\target\release\pocket_tts_ffi.dll build\
    echo Staging config...
    if not exist build\config mkdir build\config
    copy config\b6369a24.yaml build\config\
) else (
    echo Build failed.
)
