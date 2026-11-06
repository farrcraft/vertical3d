#!/usr/bin/env bash
# Checks vcpkg out at the manifest's baseline under /work, for the scripts beside this one to
# source. It sets vcpkg to the checkout and VCPKG_DEFAULT_BINARY_CACHE to the binary cache.
#
# vcpkg resolves the baseline in vcpkg.json against the checkout that runs the install, so the
# checkout is the baseline commit itself. With no version overrides, that commit holds every
# port the install reads. A checkout already at the baseline is kept.

export VCPKG_DEFAULT_BINARY_CACHE="${VCPKG_DEFAULT_BINARY_CACHE:-/work/vcpkg-cache}"
mkdir -p "$VCPKG_DEFAULT_BINARY_CACHE"

baseline=$(jq -r '."builtin-baseline"' /src/vcpkg.json)
vcpkg=/work/vcpkg
if [ "$(git -C "$vcpkg" rev-parse HEAD 2>/dev/null)" != "$baseline" ] || [ ! -x "$vcpkg/vcpkg" ]; then
    rm -rf "$vcpkg"
    git init --quiet "$vcpkg"
    git -C "$vcpkg" fetch --quiet --depth 1 https://github.com/microsoft/vcpkg.git "$baseline"
    git -C "$vcpkg" checkout --quiet --detach FETCH_HEAD
    "$vcpkg/bootstrap-vcpkg.sh" -disableMetrics
fi
