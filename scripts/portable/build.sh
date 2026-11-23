#!/usr/bin/env bash
# Builds and tests the api libraries that need neither a window nor a device, and moya, with
# Clang on Linux and under the sanitizers, and fuzzes the fuzz targets. It runs inside the image
# the Dockerfile in this directory defines, in CI and locally.
#
# One argument names the stage to run, so CI runs each stage in a job of its own:
#
#   libraries   the api libraries, built and their suites run
#   moya        moya, built and its suite run
#   rgb         moya's RGB build, built and its suite run
#   fuzz        the fuzz targets, built alone and each one fuzzed
#
# With no argument every stage runs, one after another.
#
# The tree is mounted read-only at /src. Everything the build writes goes under /work: the
# vcpkg checkout, the binary cache, a build directory for each stage and the fuzzing corpus.
# VCPKG_DEFAULT_BINARY_CACHE moves the binary cache somewhere else, so CI can keep it on the
# runner between runs.
set -euo pipefail

src=/src
work=/work
stage="${1:-all}"

# shellcheck source=vcpkg.sh
source /usr/local/bin/v3d-vcpkg

"$CXX" --version

# Configures a build directory under the sanitizers. The manifest's default features are moya's
# packages, the window, the device, the audio and OpenUSD. Only moya's are used here, so the rest
# are left out and never built. The tree is built under AddressSanitizer and the undefined
# behaviour sanitizer, so every test and fuzz target runs under both.
#
# @param 1 the build directory
# @param ... the options that set the stage apart
configure() {
    local build="$1"
    shift
    cmake -S "$src" -B "$build" -G Ninja \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_TOOLCHAIN_FILE="$vcpkg/scripts/buildsystems/vcpkg.cmake" \
        -DVCPKG_MANIFEST_NO_DEFAULT_FEATURES=ON \
        -DVCPKG_MANIFEST_FEATURES=moya \
        -DV3D_BUILD_APPS=OFF \
        -DV3D_BUILD_TESTS=ON \
        -DV3D_SANITIZE=ON \
        "$@"
}

# The libraries whose closure names neither SDL3, Vulkan nor OpenUSD. ctest runs as many suites
# at once as the machine has cores.
stage_libraries() {
    local libraries="asset;asset_media;brep;curve;database;ecs;event;font;grid;image;interchange;log;rib;type"
    configure "$work/build" -DV3D_BUILD_MOYA=OFF "-DV3D_LIBRARIES=$libraries"
    cmake --build "$work/build"
    ctest --test-dir "$work/build" --output-on-failure -j "$(nproc)"
}

# Builds everything under moya/ and runs moya's suite. The configure adds the api libraries moya
# links, whose own suites the libraries stage runs.
#
# @param 1 the build directory
# @param ... the options that set the build apart
build_moya() {
    local build="$1"
    shift
    configure "$build" -DV3D_BUILD_MOYA=ON "-DV3D_LIBRARIES=log" "$@"
    cmake --build "$build" --target moya/all
    ctest --test-dir "$build/moya" --output-on-failure -j "$(nproc)"
}

stage_moya() {
    build_moya "$work/moya"
}

# moya's RGB build, whose colour is ACEScg's three channels rather than four wavelengths, so that
# both colour types keep building and moya's suite passes in each.
stage_rgb() {
    build_moya "$work/rgb" -DV3D_MOYA_RGB=ON
}

# The fuzz targets are built alone, instrumented for libFuzzer, from the libraries that declare
# one. Each target runs for FUZZ_SECONDS, starting from its fixtures. What it finds that reaches
# new code is kept under $work/corpus, so a local run carries on from the last one. A failure
# stops the script, and libFuzzer prints the input that caused it, which belongs in the
# library's tests once the fault is fixed.
stage_fuzz() {
    configure "$work/fuzz" -DV3D_BUILD_MOYA=OFF -DV3D_FUZZ=ON "-DV3D_LIBRARIES=image;rib"
    # Each ctest entry fuzz_<name> runs the target v3dfuzz_<name> over its corpus. ctest names
    # a target's program only once it is built.
    local names
    mapfile -t names < <(ctest --test-dir "$work/fuzz" -L '^fuzz$' --show-only=json-v1 \
        | jq -r '.tests[].name | ltrimstr("fuzz_")')
    cmake --build "$work/fuzz" --target "${names[@]/#/v3dfuzz_}"

    local fuzz_seconds="${FUZZ_SECONDS:-60}"
    local name command
    for name in "${names[@]}"; do
        # the program, then -runs=0, then the corpus directories
        mapfile -t command < <(ctest --test-dir "$work/fuzz" -R "^fuzz_${name}\$" --show-only=json-v1 \
            | jq -r '.tests[0].command[]')
        mkdir -p "$work/corpus/v3dfuzz_$name" "$work/crashes"
        "${command[0]}" -max_total_time="$fuzz_seconds" -print_final_stats=1 \
            -artifact_prefix="$work/crashes/v3dfuzz_$name-" "$work/corpus/v3dfuzz_$name" "${command[@]:2}"
    done
}

case "$stage" in
    libraries | moya | rgb | fuzz) "stage_$stage" ;;
    all)
        stage_libraries
        stage_moya
        stage_rgb
        stage_fuzz
        ;;
    *)
        echo "Unknown stage $stage: give libraries, moya, rgb, fuzz or nothing" >&2
        exit 2
        ;;
esac
