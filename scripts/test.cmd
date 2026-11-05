@echo off
rem Run the Boost.Test suites through ctest, in a developer environment for the same
rem reason scripts\build.cmd needs one. Anything after the script name is passed to
rem ctest, so a single suite is scripts\test.cmd -R input.

setlocal

call "%~dp0vcvars.cmd" || exit /b 1

rem ctest finds no tests in a tree that is not configured, and reports that as a success
set "V3D_BUILD=%~dp0..\out\build\x64-Debug"
if not exist "%V3D_BUILD%\build.ninja" (
    echo %V3D_BUILD% is not configured. Run cmake --preset x64-Debug first.
    exit /b 1
)

ctest --test-dir "%V3D_BUILD%" --output-on-failure %*
