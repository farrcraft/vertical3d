v3d_add_test(type
	"TestMain.cxx"
	"3dtypesTest.cxx"
	"AABBoxTest.cxx"
	"ArcBallTest.cxx"
	"Bound2DTest.cxx"
	"CameraTest.cxx"
	"CameraProfileTest.cxx"
	"RayTest.cxx")

target_link_libraries(v3dtest_type PRIVATE v3dlib_type)
