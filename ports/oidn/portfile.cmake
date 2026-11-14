# A shared build loads its CPU device from a module beside the library at run time, which an
# executable's dependencies do not name. A static build links the device in.
vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

# The release's source archive holds the trained weights, which the repository keeps in a
# submodule that GitHub's archive of a tag leaves out.
vcpkg_download_distfile(archive_path
    URLS "https://github.com/RenderKit/oidn/releases/download/v${VERSION}/oidn-${VERSION}.src.tar.gz"
    FILENAME "oidn-${VERSION}.src.tar.gz"
    SHA512 fee6ff6d8b0b496965ef805e9aa009a087122e26151ef34e0147d7c60fbb8edffb2492d7d0a02ad66e2dec6fd5bbd788598f453936f155e66a4016d7ed858ee3
)
vcpkg_extract_source_archive(SOURCE_PATH ARCHIVE "${archive_path}")

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DOIDN_STATIC_LIB=ON
        -DOIDN_DEVICE_CPU=ON
        -DOIDN_DEVICE_SYCL=OFF
        -DOIDN_DEVICE_CUDA=OFF
        -DOIDN_DEVICE_HIP=OFF
        -DOIDN_DEVICE_METAL=OFF
        -DOIDN_FILTER_RT=ON
        -DOIDN_FILTER_RTLIGHTMAP=OFF
        -DOIDN_APPS=OFF
        -DOIDN_INSTALL_DEPENDENCIES=OFF
        "-DISPC_EXECUTABLE=${CURRENT_HOST_INSTALLED_DIR}/tools/vcpkg-tool-ispc/bin/ispc${VCPKG_HOST_EXECUTABLE_SUFFIX}"
)
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH "lib/cmake/OpenImageDenoise-${VERSION}" PACKAGE_NAME OpenImageDenoise)

file(REMOVE_RECURSE
    "${CURRENT_PACKAGES_DIR}/debug/include"
    "${CURRENT_PACKAGES_DIR}/debug/share"
    "${CURRENT_PACKAGES_DIR}/share/doc"
)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.txt" "${SOURCE_PATH}/third-party-programs.txt")
