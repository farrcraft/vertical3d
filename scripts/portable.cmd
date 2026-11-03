@echo off
rem Run the portable CI job locally: build the api libraries that need neither a window nor a
rem device with Clang on Linux, and run their tests. It builds the image the job builds in, from
rem scripts\portable\Dockerfile, and runs scripts\portable\build.sh in it, as the job does.
rem Docker Desktop has to be running.
rem
rem   scripts\portable.cmd
rem
rem The tree is mounted read-only. The build, the vcpkg checkout and the binary cache live in
rem the Docker volume v3d-portable, so a second run reuses what the first one built.
rem "docker volume rm v3d-portable" starts again from cold.

setlocal

docker build -t v3d-portable "%~dp0portable" || exit /b 1
docker run --rm -v "%~dp0..:/src:ro" -v v3d-portable:/work v3d-portable
