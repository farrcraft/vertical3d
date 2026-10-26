# Clips, poses, curves and tracks are plain values. The tests need no window, no device, and no
# files copied beside the executable.
v3d_add_test(animation
	"TestMain.cxx"
	"AnimationTest.cxx"
	"CurveTest.cxx"
	"SpriteClipTest.cxx"
	"TrackTest.cxx")

target_link_libraries(v3dtest_animation PRIVATE v3dlib_animation)
