@echo off
rem Run a portable CI job locally, in the image it builds in, from scripts\portable\Dockerfile.
rem Docker Desktop has to be running.
rem
rem   scripts\portable.cmd              The libraries that need no window or device.
rem   scripts\portable.cmd whole        The whole tree with Clang, and every suite.
rem   scripts\portable.cmd whole gcc    The whole tree with GCC, and every suite.
rem
rem The first builds with Clang under the sanitizers, then fuzzes the fuzz targets.
rem
rem The tree is mounted read-only. The builds, the vcpkg checkout and the binary cache live in
rem the Docker volume v3d-portable, so a second run reuses what the first one built.
rem "docker volume rm v3d-portable" starts again from cold.

setlocal

set "script=/usr/local/bin/v3d-portable"
set "compiler="
if /i "%~1"=="whole" set "script=/usr/local/bin/v3d-whole"
if /i "%~2"=="gcc" set "compiler=-e CC=gcc -e CXX=g++"

docker build -t v3d-portable "%~dp0portable" || exit /b 1
docker run --rm %compiler% -v "%~dp0..:/src:ro" -v v3d-portable:/work v3d-portable bash %script%
