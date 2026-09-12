@echo off
rem Run the Boost.Test suites through ctest, in a developer environment for the same
rem reason scripts\build.cmd needs one. Anything after the script name is passed to
rem ctest, so a single suite is scripts\test.cmd -R input.

setlocal

set "V3D_VCVARS=%ProgramFiles%\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%V3D_VCVARS%" set "V3D_VCVARS=%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%V3D_VCVARS%" (
    echo Could not find vcvars64.bat. Set V3D_VCVARS to it, or see docs/Build.md.
    exit /b 1
)

call "%V3D_VCVARS%" >nul || exit /b 1

ctest --test-dir "%~dp0..\out\build\x64-Debug" --output-on-failure %*
