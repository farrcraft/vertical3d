@echo off
rem Enter the Visual Studio developer environment, which puts the compiler, the linker, ninja,
rem cmake and ctest on PATH. scripts\build.cmd, scripts\test.cmd and the fails-first harness call
rem this file. It has no setlocal, so the environment it enters stays with its caller.
rem
rem V3D_VCVARS names vcvars64.bat when it is set. Otherwise the Community edition of Visual
rem Studio 18 is tried, then that of Visual Studio 2022.

if not defined V3D_VCVARS set "V3D_VCVARS=%ProgramFiles%\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%V3D_VCVARS%" set "V3D_VCVARS=%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%V3D_VCVARS%" (
    echo Could not find vcvars64.bat. Set V3D_VCVARS to it, or see docs/contributing/Build.md. 1>&2
    exit /b 1
)

call "%V3D_VCVARS%" >nul || exit /b 1
