v3d_add_api_library(ecs
	"component/Position2D.h" "component/Position2D.cpp"
 "component/Position1D.h" "component/Position1D.cpp"
 "component/Color3.h"
 "component/Transform.h" "component/Transform.cpp"
 "component/Playback.h" "component/Playback.cpp"
 "component/Emitter.h" "component/Emitter.cpp"
 "Previous.h"
 "System.h" "System.cpp")

target_link_libraries(v3dlib_ecs PUBLIC v3dlib_type glm::glm EnTT::EnTT)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
