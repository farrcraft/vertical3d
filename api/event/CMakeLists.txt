v3d_add_api_library(event
	"kind/Key.h" "kind/Key.cpp"
	"kind/KeyDown.h" "kind/KeyDown.cpp"
	"kind/KeyUp.h" "kind/KeyUp.cpp"
	"kind/MouseWheel.h" "kind/MouseWheel.cpp"
	"kind/TextInput.h" "kind/TextInput.cpp"
 "kind/WindowResize.h" "kind/WindowResize.cpp"
 "kind/WindowFocus.h" "kind/WindowFocus.cpp"
 "kind/Sound.h" "kind/Sound.cpp"
 "Event.h" "Event.cpp"
 "Mapper.h" "Mapper.cpp"
 "Engine.h" "Engine.cpp"
 "Context.h" "Context.cpp"
 "kind/MouseButton.h" "kind/MouseButton.cpp"
 "kind/MouseMotion.h" "kind/MouseMotion.cpp"
 "State.h"
 "Type.h")

target_link_libraries(v3dlib_event PUBLIC glm::glm EnTT::EnTT)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
