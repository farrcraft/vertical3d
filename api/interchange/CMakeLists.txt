v3d_add_api_library(interchange
	"Scene.h"
	"Placement.h" "Placement.cxx"
	"Gltf.h" "Gltf.cxx"
	"CgltfImpl.cxx")

# PUBLIC: a scene places each node by a type::Transform and holds curve::Curve values. glm, for
# the points and values.
target_link_libraries(v3dlib_interchange PUBLIC v3dlib_curve v3dlib_type glm::glm)

# PRIVATE: the glTF reader reads its document and buffers through asset::readFile.
target_link_libraries(v3dlib_interchange PRIVATE v3dlib_asset)

# PRIVATE: only the glTF reader and the translation unit owning cgltf's implementation include
# the header, and no header here names a cgltf type. The implementation is compiled here, once
# for every program, and asset_media's model loader links this library to reach it.
target_include_directories(v3dlib_interchange PRIVATE "${V3D_CGLTF_INCLUDE_DIR}")

# cgltf is a third party header compiled here rather than vendored into the tree. Its
# implementation half is not held to /W4 or to either analyser, because a local fix would be
# lost the next time the port updates. The pragma in the file drops the warning level,
# /analyze- takes it out of MSVC's analysis and SKIP_LINTING out of clang-tidy's.
set_source_files_properties("CgltfImpl.cxx" PROPERTIES SKIP_LINTING TRUE)
if(MSVC)
	set_source_files_properties("CgltfImpl.cxx" PROPERTIES COMPILE_OPTIONS "/analyze-")
else()
	# -w is what the pragma does for MSVC, which Clang and GCC take from the command line
	set_source_files_properties("CgltfImpl.cxx" PROPERTIES COMPILE_OPTIONS "-w")
endif()

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
