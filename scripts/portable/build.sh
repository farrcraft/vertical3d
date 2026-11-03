#!/usr/bin/env bash
# Builds and tests the api libraries that need neither a window nor a device, with Clang on
# Linux. It runs inside the image the Dockerfile in this directory defines, in CI and locally.
#
# The tree is mounted read-only at /src. Everything the build writes goes under /work: the
# vcpkg checkout, the binary cache, and the build directory. VCPKG_DEFAULT_BINARY_CACHE moves
# the binary cache somewhere else, so CI can keep it on the runner between runs.
set -euo pipefail

src=/src
work=/work
export VCPKG_DEFAULT_BINARY_CACHE="${VCPKG_DEFAULT_BINARY_CACHE:-$work/vcpkg-cache}"

# The libraries whose closure names neither SDL3, Vulkan nor OpenUSD
libraries="asset;asset_media;brep;curve;database;ecs;event;font;grid;image;interchange;log;render_offline;type"

mkdir -p "$VCPKG_DEFAULT_BINARY_CACHE"

# vcpkg resolves the baseline in vcpkg.json against the checkout that runs the install, so the
# checkout is the baseline commit itself. With no version overrides, that commit holds every
# port the install reads. A checkout already at the baseline is kept.
baseline=$(jq -r '."builtin-baseline"' "$src/vcpkg.json")
vcpkg="$work/vcpkg"
if [ "$(git -C "$vcpkg" rev-parse HEAD 2>/dev/null)" != "$baseline" ] || [ ! -x "$vcpkg/vcpkg" ]; then
    rm -rf "$vcpkg"
    git init --quiet "$vcpkg"
    git -C "$vcpkg" fetch --quiet --depth 1 https://github.com/microsoft/vcpkg.git "$baseline"
    git -C "$vcpkg" checkout --quiet --detach FETCH_HEAD
    "$vcpkg/bootstrap-vcpkg.sh" -disableMetrics
fi

"$CXX" --version

# The manifest's default features are the window, the device, the audio and OpenUSD, which
# these libraries do not use, so they are left out and never built.
cmake -S "$src" -B "$work/build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_TOOLCHAIN_FILE="$vcpkg/scripts/buildsystems/vcpkg.cmake" \
    -DVCPKG_MANIFEST_NO_DEFAULT_FEATURES=ON \
    -DV3D_BUILD_APPS=OFF \
    -DV3D_BUILD_TESTS=ON \
    "-DV3D_LIBRARIES=$libraries"
cmake --build "$work/build"
ctest --test-dir "$work/build" --output-on-failure
