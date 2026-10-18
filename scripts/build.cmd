@echo off
rem Build the tree from a plain shell. The compiler, the linker and ninja are only on a
rem developer environment's PATH, so this script enters one first. Visual Studio
rem configures out/build/x64-Debug from CMakeSettings.json and does not need this script.
rem
rem   scripts\build.cmd                 everything
rem   scripts\build.cmd pong            one target by name
rem
rem Anything after the script name is passed to ninja. The tree must already be configured,
rem because this script does not run cmake.

setlocal

if not defined V3D_VCVARS set "V3D_VCVARS=%ProgramFiles%\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%V3D_VCVARS%" set "V3D_VCVARS=%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%V3D_VCVARS%" (
    echo Could not find vcvars64.bat. Set V3D_VCVARS to it, or see docs/contributing/Build.md.
    exit /b 1
)

call "%V3D_VCVARS%" >nul || exit /b 1

set "V3D_BUILD=%~dp0..\out\build\x64-Debug"
if not exist "%V3D_BUILD%\build.ninja" (
    echo %V3D_BUILD% is not configured. docs/contributing/Build.md has the cmake line.
    exit /b 1
)

ninja -C "%V3D_BUILD%" %*
