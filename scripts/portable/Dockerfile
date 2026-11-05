# The image the portable job builds and tests in, and the one that reproduces the job locally.
# It holds the compiler and the tools vcpkg builds the dependencies with. The tree is mounted
# into it, and build.sh in this directory does the build.
FROM ubuntu:26.04

# The oldest Clang the tree supports. A newer Clang accepts code an older one rejects, so the
# job builds with the oldest, which is the nearest to the Clang a consumer on macOS has.
ARG CLANG=20

# vcpkg builds ICU with autotools, and thorvg with Meson, which needs Python. build-essential
# brings libstdc++, which Clang compiles against on Linux. libclang-rt holds the sanitizer and
# libFuzzer runtimes, which Clang links but does not install itself. llvm holds the symbolizer
# that names the functions in a sanitizer's report.
RUN apt-get update && apt-get install -y --no-install-recommends \
    autoconf \
    autoconf-archive \
    automake \
    build-essential \
    ca-certificates \
    clang-${CLANG} \
    cmake \
    curl \
    git \
    jq \
    libclang-rt-${CLANG}-dev \
    libtool \
    llvm-${CLANG} \
    ninja-build \
    pkg-config \
    python3 \
    tar \
    unzip \
    zip \
    && rm -rf /var/lib/apt/lists/*

ENV CC=clang-${CLANG} CXX=clang++-${CLANG} \
    ASAN_SYMBOLIZER_PATH=/usr/bin/llvm-symbolizer-${CLANG} \
    UBSAN_OPTIONS=print_stacktrace=1

COPY build.sh /usr/local/bin/v3d-portable
CMD ["bash", "/usr/local/bin/v3d-portable"]
