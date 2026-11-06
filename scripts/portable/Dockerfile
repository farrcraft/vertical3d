# The image the portable jobs build and test in, and the one that reproduces them locally. It
# holds the compilers and the tools vcpkg builds the dependencies with. The tree is mounted into
# it. build.sh in this directory builds the libraries that need neither a window nor a device,
# and whole.sh builds the whole tree.
FROM ubuntu:26.04

# The oldest Clang the tree supports. A newer Clang accepts code an older one rejects, so the
# job builds with the oldest, which is the nearest to the Clang a consumer on macOS has.
ARG CLANG=20

# vcpkg builds ICU with autotools, and thorvg with Meson, which needs Python. build-essential
# brings GCC and libstdc++, which Clang compiles against on Linux. libclang-rt holds the
# sanitizer and libFuzzer runtimes, which Clang links but does not install itself. llvm holds the
# symbolizer that names the functions in a sanitizer's report.
#
# The whole tree needs more. SDL3 builds its X11, Wayland and audio backends against the
# system's headers. glslc compiles the shaders. mesa-vulkan-drivers holds lavapipe, the software
# Vulkan driver the device suites draw with, and vulkan-validationlayers the layer they require to
# be on. xvfb is the display the window suites open on, and openbox the window manager that
# carries out a window's request to go full screen, as a desktop's does.
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
    glslc \
    jq \
    libasound2-dev \
    libclang-rt-${CLANG}-dev \
    libdbus-1-dev \
    libdecor-0-dev \
    libdrm-dev \
    libegl-dev \
    libgbm-dev \
    libgl-dev \
    libpulse-dev \
    libtool \
    libudev-dev \
    libwayland-dev \
    libx11-dev \
    libx11-xcb-dev \
    libxcb1-dev \
    libxcursor-dev \
    libxext-dev \
    libxfixes-dev \
    libxi-dev \
    libxkbcommon-dev \
    libxrandr-dev \
    libxss-dev \
    libxtst-dev \
    llvm-${CLANG} \
    mesa-vulkan-drivers \
    ninja-build \
    openbox \
    pkg-config \
    python3 \
    tar \
    unzip \
    vulkan-validationlayers \
    wayland-protocols \
    xauth \
    xvfb \
    zip \
    && rm -rf /var/lib/apt/lists/*

ENV CC=clang-${CLANG} CXX=clang++-${CLANG} \
    ASAN_SYMBOLIZER_PATH=/usr/bin/llvm-symbolizer-${CLANG} \
    UBSAN_OPTIONS=print_stacktrace=1

COPY vcpkg.sh /usr/local/bin/v3d-vcpkg
COPY build.sh /usr/local/bin/v3d-portable
COPY whole.sh /usr/local/bin/v3d-whole
CMD ["bash", "/usr/local/bin/v3d-portable"]
