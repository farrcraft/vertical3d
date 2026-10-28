# The components and System are plain data over an entt registry. The tests need no window, no
# device, and no files copied beside the executable.
v3d_add_test(ecs
	"TestMain.cpp"
	"EffectTest.cxx"
	"EmitterTest.cpp"
	"PlaybackTest.cpp"
	"Positions.h"
	"PreviousTest.cpp"
	"RefTest.cpp"
	"SystemTest.cpp"
	"TransformTest.cpp"
	"WeatherTest.cxx")

target_link_libraries(v3dtest_ecs PRIVATE v3dlib_ecs)
