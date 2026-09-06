v3d_add_api_library(event
	"Key.h" "Key.cpp"
	"KeyDown.h" "KeyDown.cpp"
	"KeyUp.h" "KeyUp.cpp"
 "WindowResize.h" "WindowResize.cpp"
 "Sound.h" "Sound.cpp"
 "Event.h" "Event.cpp"
 "Mapper.h" "Mapper.cpp"
 "Engine.h" "Engine.cpp"
 "Context.h" "Context.cpp"
 "MouseButton.h" "MouseButton.cpp"
 "MouseMotion.h" "MouseMotion.cpp"
 "State.h"
 "Type.h")

target_link_libraries(v3dlib_event PUBLIC glm::glm EnTT::EnTT)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
