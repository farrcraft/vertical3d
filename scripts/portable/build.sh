#!/usr/bin/env bash
# Builds and tests the api libraries that need neither a window nor a device, with Clang on
# Linux and under the sanitizers, then fuzzes the fuzz targets. It runs inside the image the
# Dockerfile in this directory defines, in CI and locally.
#
# The tree is mounted read-only at /src. Everything the build writes goes under /work: the
# vcpkg checkout, the binary cache, the build directory and the fuzzing corpus.
# VCPKG_DEFAULT_BINARY_CACHE moves the binary cache somewhere else, so CI can keep it on the
# runner between runs.
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
# these libraries do not use, so they are left out and never built. The tree is built under
# AddressSanitizer and the undefined behaviour sanitizer, so every test runs under both, and
# instrumented for libFuzzer, so the fuzz targets can run below.
cmake -S "$src" -B "$work/build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_TOOLCHAIN_FILE="$vcpkg/scripts/buildsystems/vcpkg.cmake" \
    -DVCPKG_MANIFEST_NO_DEFAULT_FEATURES=ON \
    -DV3D_BUILD_APPS=OFF \
    -DV3D_BUILD_TESTS=ON \
    -DV3D_SANITIZE=ON \
    -DV3D_FUZZ=ON \
    "-DV3D_LIBRARIES=$libraries"
cmake --build "$work/build"
ctest --test-dir "$work/build" --output-on-failure

# Each fuzz target runs for FUZZ_SECONDS, starting from its fixtures. What it finds that reaches
# new code is kept under $work/corpus, so a local run carries on from the last one. A failure
# stops the script, and libFuzzer prints the input that caused it, which belongs in the
# library's tests once the fault is fixed.
fuzz_seconds="${FUZZ_SECONDS:-60}"
while IFS= read -r -d '' target; do
    name=$(basename "$target")
    seeds=$(ctest --test-dir "$work/build" -R "^fuzz_${name#v3dfuzz_}\$" --show-only=json-v1 \
        | jq -r '.tests[0].command[2:][]')
    mkdir -p "$work/corpus/$name" "$work/crashes"
    # shellcheck disable=SC2086
    "$target" -max_total_time="$fuzz_seconds" -print_final_stats=1 \
        -artifact_prefix="$work/crashes/$name-" "$work/corpus/$name" $seeds
done < <(find "$work/build" -type f -name 'v3dfuzz_*' -perm -u+x -print0 | sort -z)
