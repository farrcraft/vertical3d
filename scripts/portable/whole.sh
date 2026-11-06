#!/usr/bin/env bash
# Builds the whole tree, the apps and every api library, with the compiler CC and CXX name, and
# runs every suite. The device suites draw with lavapipe, Mesa's software Vulkan driver, and the
# suites that open a window open it on a virtual X display. It runs inside the image the
# Dockerfile in this directory defines, in CI and locally.
#
# The tree is mounted read-only at /src. Everything the build writes goes under /work, in a build
# directory named for the compiler, so a Clang build and a GCC build sit side by side.
set -euo pipefail

# shellcheck source=vcpkg.sh
source /usr/local/bin/v3d-vcpkg

"$CXX" --version
build="/work/whole-$(basename "$CXX")"

cmake -S /src -B "$build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_TOOLCHAIN_FILE="$vcpkg/scripts/buildsystems/vcpkg.cmake" \
    -DV3D_BUILD_APPS=ON \
    -DV3D_BUILD_TESTS=ON
cmake --build "$build"

# lavapipe is the only driver the loader is offered, so a suite cannot pick up another
export VK_DRIVER_FILES
VK_DRIVER_FILES=$(find /usr/share/vulkan/icd.d -name 'lvp_icd*.json' | head -n 1)
# The container has no sound server, so SDL plays into its dummy driver, as on a machine with no
# sound card. SDL wants a runtime directory for the display.
export SDL_AUDIO_DRIVER=dummy
export XDG_RUNTIME_DIR=/tmp/xdg-runtime
mkdir -p -m 700 "$XDG_RUNTIME_DIR"
# the window manager starts on the virtual display before the suites, and goes with it
xvfb-run --auto-servernum bash -c "openbox & sleep 1; ctest --test-dir '$build' --output-on-failure"
