v3d_add_test(type
	"TestMain.cxx"
	"AABBoxTest.cxx"
	"AnimationTest.cxx"
	"ArcBallTest.cxx"
	"Bound2DTest.cxx"
	"CameraTest.cxx"
	"FrustumTest.cxx"
	"ProfileTest.cxx"
	"IsometricTest.cxx"
	"PlaneTest.cxx"
	"RayTest.cxx")

target_link_libraries(v3dtest_type PRIVATE v3dlib_type)
