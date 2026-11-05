# The source that holds the version is written at build time by cmake/v3dVersion.cmake, so a
# build names the commit it was built from even when nothing has configured since. The target
# that writes it runs on every build, and leaves the file alone when the version is the same.
find_package(Git QUIET)
set(v3d_version_source "${CMAKE_CURRENT_BINARY_DIR}/generated/Version.cpp")
add_custom_target(v3d_version_source
	COMMAND ${CMAKE_COMMAND}
		"-DOUTPUT=${v3d_version_source}"
		"-DNUMBER=${vertical3d_VERSION}"
		"-DRELEASE=${V3D_RELEASE}"
		"-DPRERELEASE=${V3D_PRERELEASE}"
		"-DGIT=${GIT_EXECUTABLE}"
		"-DROOT=${V3D_ROOT}"
		-P "${V3D_ROOT}/cmake/v3dVersion.cmake"
	BYPRODUCTS "${v3d_version_source}"
	COMMENT "Naming the build's version"
	VERBATIM)

v3d_add_api_library(version
	"Version.h" "${v3d_version_source}"
)
add_dependencies(v3dlib_version v3d_version_source)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
