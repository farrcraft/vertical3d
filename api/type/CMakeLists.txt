v3d_add_api_library(type
"3dtypes.h"
"AABBox.h" "AABBox.cxx"
"ArcBall.h" "ArcBall.cxx"
"Bound2D.h" "Bound2D.cxx"
"Camera.h" "Camera.cxx"
"CameraProfile.h" "CameraProfile.cxx"
"Ray.h" "Ray.cxx"
"String.h" "String.cxx"
)

target_link_libraries(v3dlib_type PUBLIC glm::glm)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
