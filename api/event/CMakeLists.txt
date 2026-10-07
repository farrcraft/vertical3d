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
 "Bindings.h" "Bindings.cpp"
 "Source.h" "Source.cpp"
 "Context.h" "Context.cpp"
 "kind/MouseButton.h" "kind/MouseButton.cpp"
 "kind/MouseMotion.h" "kind/MouseMotion.cpp"
 "State.h"
 "Type.h")

# v3dlib_log and Boost::json: Bindings.h holds the logger it reports a document through, and
# the boost::json::object it was read from.
target_link_libraries(v3dlib_event PUBLIC v3dlib_log glm::glm EnTT::EnTT Boost::json)

# PRIVATE: only Bindings.cpp includes the checked JSON reads.
target_link_libraries(v3dlib_event PRIVATE v3dlib_asset)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
