v3d_add_test(type
	"TestMain.cxx"
	"AABBoxTest.cxx"
	"ArcBallTest.cxx"
	"Bound2DTest.cxx"
	"CameraTest.cxx"
	"CheckedTest.cxx"
	"FlagsTest.cxx"
	"FrustumTest.cxx"
	"ProfileTest.cxx"
	"IsometricTest.cxx"
	"PlaneTest.cxx"
	"RandomTest.cxx"
	"RayTest.cxx")

target_link_libraries(v3dtest_type PRIVATE v3dlib_type)
