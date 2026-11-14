# ISPC builds with its own LLVM, so the port installs the compiler its release builds rather than
# building it. The compiler runs on the host and ships in nothing it compiles.
set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)
set(VCPKG_POLICY_SKIP_ARCHITECTURE_CHECK enabled)
set(VCPKG_BUILD_TYPE release)

if(VCPKG_TARGET_IS_WINDOWS)
    set(archive "ispc-v${VERSION}-windows.zip")
    set(sha512 732fb5702cc8f2d77975a87b21ef697384d9b7d49f75086b3a89fdf39f155584340719012e3721e42d8cb5945b8b85bfea6b182a7afbc701a1af6e7e9b85d980)
    set(folder "ispc-v${VERSION}-windows")
elseif(VCPKG_TARGET_IS_LINUX)
    set(archive "ispc-v${VERSION}-linux.tar.gz")
    set(sha512 b99db7514971795673b707b4647813230f9b90f12b6f3f2f4cd9339c450a6d76433ef436a70487094d7deb77e8e5f6fa3be8b759ed5d2da38441a421b4f4d3fa)
    set(folder "ispc-v${VERSION}-linux")
else()
    message(FATAL_ERROR "ISPC's releases are taken only for Windows and Linux on x64")
endif()

vcpkg_download_distfile(archive_path
    URLS "https://github.com/ispc/ispc/releases/download/v${VERSION}/${archive}"
    FILENAME "${archive}"
    SHA512 ${sha512}
)

set(release "${CURRENT_BUILDTREES_DIR}/release")
file(REMOVE_RECURSE "${release}")
vcpkg_extract_archive(ARCHIVE "${archive_path}" DESTINATION "${release}")
# The compiler finds its standard headers in ../include, and on Linux its libraries in ../lib, so
# the release's layout is kept
foreach(part IN ITEMS bin include lib)
    file(INSTALL "${release}/${folder}/${part}" DESTINATION "${CURRENT_PACKAGES_DIR}/tools/${PORT}")
endforeach()

vcpkg_install_copyright(FILE_LIST "${release}/${folder}/LICENSE.txt")
