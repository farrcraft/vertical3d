@echo off
rem Build the tree from a plain shell. The compiler, the linker and ninja are only on a
rem developer environment's PATH, so this script enters one first. Visual Studio
rem configures out/build/x64-Debug from the x64-Debug preset and does not need this script.
rem
rem   scripts\build.cmd                 everything
rem   scripts\build.cmd pong            one target by name
rem
rem Anything after the script name is passed to ninja. The tree must already be configured,
rem because this script does not run cmake. cmake --preset x64-Debug configures it.

setlocal

call "%~dp0vcvars.cmd" || exit /b 1

set "V3D_BUILD=%~dp0..\out\build\x64-Debug"
if not exist "%V3D_BUILD%\build.ninja" (
    echo %V3D_BUILD% is not configured. Run cmake --preset x64-Debug first.
    exit /b 1
)

ninja -C "%V3D_BUILD%" %*
