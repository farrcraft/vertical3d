@echo off
rem Run the Boost.Test suites through ctest, in a developer environment for the same
rem reason scripts\build.cmd needs one. Anything after the script name is passed to
rem ctest, so a single suite is scripts\test.cmd -R input.

setlocal

if not defined V3D_VCVARS set "V3D_VCVARS=%ProgramFiles%\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%V3D_VCVARS%" set "V3D_VCVARS=%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%V3D_VCVARS%" (
    echo Could not find vcvars64.bat. Set V3D_VCVARS to it, or see docs/contributing/Build.md.
    exit /b 1
)

call "%V3D_VCVARS%" >nul || exit /b 1

rem ctest finds no tests in a tree that is not configured, and reports that as a success
set "V3D_BUILD=%~dp0..\out\build\x64-Debug"
if not exist "%V3D_BUILD%\build.ninja" (
    echo %V3D_BUILD% is not configured. docs/contributing/Build.md has the cmake line.
    exit /b 1
)

ctest --test-dir "%V3D_BUILD%" --output-on-failure %*
