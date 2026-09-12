@echo off
rem Build the tree from a plain shell, which ninja cannot do on its own: the compiler,
rem the linker and ninja itself are all on the developer environment's PATH and on
rem nothing else's. Visual Studio configures out/build/x64-Debug from CMakeSettings.json
rem and needs none of this; a shell does.
rem
rem   scripts\build.cmd                 everything
rem   scripts\build.cmd pong            one target by name
rem
rem Anything after the script name is passed to ninja. docs/Build.md covers configuring
rem a cold tree, which this does not do - it builds one that is already configured.

setlocal

set "V3D_VCVARS=%ProgramFiles%\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%V3D_VCVARS%" set "V3D_VCVARS=%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%V3D_VCVARS%" (
    echo Could not find vcvars64.bat. Set V3D_VCVARS to it, or see docs/Build.md.
    exit /b 1
)

call "%V3D_VCVARS%" >nul || exit /b 1

set "V3D_BUILD=%~dp0..\out\build\x64-Debug"
if not exist "%V3D_BUILD%\build.ninja" (
    echo %V3D_BUILD% is not configured. docs/Build.md has the cmake line.
    exit /b 1
)

ninja -C "%V3D_BUILD%" %*
