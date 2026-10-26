v3d_add_api_library(type
"Checked.h" "Checked.cxx"
"Flags.h"
"Random.h" "Random.cxx"
"Transform.h" "Transform.cxx"
"camera/ArcBall.h" "camera/ArcBall.cxx"
"camera/Camera.h" "camera/Camera.cxx"
"camera/Isometric.h" "camera/Isometric.cxx"
"camera/Profile.h" "camera/Profile.cxx"
"geometry/AABBox.h" "geometry/AABBox.cxx"
"geometry/Bound2D.h" "geometry/Bound2D.cxx"
"geometry/Frustum.h" "geometry/Frustum.cxx"
"geometry/Plane.h" "geometry/Plane.cxx"
"geometry/Line.h"
"geometry/Ray.h" "geometry/Ray.cxx"
)

target_link_libraries(v3dlib_type PUBLIC glm::glm)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
