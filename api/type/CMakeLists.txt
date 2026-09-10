v3d_add_api_library(type
"Model.h" "Model.cxx"
"camera/ArcBall.h" "camera/ArcBall.cxx"
"camera/Camera.h" "camera/Camera.cxx"
"camera/Isometric.h" "camera/Isometric.cxx"
"camera/Profile.h" "camera/Profile.cxx"
"geometry/AABBox.h" "geometry/AABBox.cxx"
"geometry/Bound2D.h" "geometry/Bound2D.cxx"
"geometry/Ray.h" "geometry/Ray.cxx"
)

target_link_libraries(v3dlib_type PUBLIC glm::glm)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
