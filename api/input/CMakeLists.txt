v3d_add_api_library(input
	"DeviceType.h"
	"Device.h" "Device.cpp"
	"Engine.h" "Engine.cpp"
	"Keyboard.h" "Keyboard.cpp"
	"KeyState.h" "KeyState.cpp"
	"Mouse.h" "Mouse.cpp"
	"MouseState.h" "MouseState.cpp"
 "DeviceType.h")

# PUBLIC: Device.h and Engine.h name SDL event types, and both dispatch through the event
# engine.
# v3dlib_type: DeviceType.h is a type::Flags.
target_link_libraries(v3dlib_input PUBLIC v3dlib_event v3dlib_type SDL3::SDL3 glm::glm EnTT::EnTT)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
