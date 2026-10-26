v3d_add_api_library(ecs
	"component/Transform.h" "component/Transform.cpp"
	"component/Playback.h" "component/Playback.cpp"
	"component/Emitter.h" "component/Emitter.cpp"
	"effect/Emitter.h" "effect/Emitter.cxx"
	"effect/Particle.h" "effect/Particle.cxx"
	"effect/State.h" "effect/State.cxx"
	"effect/Weather.h" "effect/Weather.cxx"
	"Previous.h"
	"Ref.h"
	"System.h" "System.cpp")

# PUBLIC: Playback.h holds an animation::Clock, an effect::Emitter holds animation tracks, and
# effect::State draws from a type::Random.
target_link_libraries(v3dlib_ecs PUBLIC v3dlib_animation v3dlib_type glm::glm EnTT::EnTT)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
