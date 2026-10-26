v3d_add_api_library(animation
	"Channel.h"
	"Clip.h" "Clip.cxx"
	"Clock.h" "Clock.cxx"
	"Curve.h" "Curve.cxx"
	"Pose.h" "Pose.cxx"
	"Skeleton.h" "Skeleton.cxx"
	"SpriteClip.h" "SpriteClip.cxx"
	"Track.h"
)

# PUBLIC: every header here holds glm values.
target_link_libraries(v3dlib_animation PUBLIC glm::glm)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
