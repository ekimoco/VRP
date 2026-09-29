@echo off
setlocal

cd /d "%~dp0"

if not defined PRESET set "PRESET=mingw-debug"
set "BUILD_DIR=build\%PRESET%"

if not exist "%BUILD_DIR%\build.ninja" (
    cmake --preset "%PRESET%" || exit /b
)

cmake --build --preset "%PRESET%" || exit /b

"%BUILD_DIR%\resprof.exe" %*