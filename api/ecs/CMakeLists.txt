v3d_add_api_library(ecs
	"component/Transform.h" "component/Transform.cpp"
	"component/Playback.h" "component/Playback.cpp"
	"component/Emitter.h" "component/Emitter.cpp"
	"Previous.h"
	"Ref.h"
	"System.h" "System.cpp")

target_link_libraries(v3dlib_ecs PUBLIC v3dlib_type glm::glm EnTT::EnTT)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
