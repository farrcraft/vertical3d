v3d_add_api_library(type
"Model.h" "Model.cxx"
"Random.h" "Random.cxx"
"Skeleton.h" "Skeleton.cxx"
"animation/Channel.h"
"animation/Clip.h" "animation/Clip.cxx"
"animation/Clock.h" "animation/Clock.cxx"
"animation/Pose.h" "animation/Pose.cxx"
"animation/SpriteClip.h" "animation/SpriteClip.cxx"
"animation/Track.h"
"camera/ArcBall.h" "camera/ArcBall.cxx"
"camera/Camera.h" "camera/Camera.cxx"
"camera/Isometric.h" "camera/Isometric.cxx"
"camera/Profile.h" "camera/Profile.cxx"
"effect/Emitter.h" "effect/Emitter.cxx"
"effect/Particle.h" "effect/Particle.cxx"
"effect/State.h" "effect/State.cxx"
"effect/Weather.h" "effect/Weather.cxx"
"geometry/AABBox.h" "geometry/AABBox.cxx"
"geometry/Bound2D.h" "geometry/Bound2D.cxx"
"geometry/Frustum.h" "geometry/Frustum.cxx"
"geometry/Plane.h" "geometry/Plane.cxx"
"geometry/Ray.h" "geometry/Ray.cxx"
)

target_link_libraries(v3dlib_type PUBLIC glm::glm)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
