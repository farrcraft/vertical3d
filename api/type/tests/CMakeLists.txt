v3d_add_test(type
	"TestMain.cxx"
	"AABBoxTest.cxx"
	"AnimationTest.cxx"
	"ArcBallTest.cxx"
	"Bound2DTest.cxx"
	"CameraTest.cxx"
	"EffectTest.cxx"
	"FrustumTest.cxx"
	"ProfileTest.cxx"
	"IsometricTest.cxx"
	"PlaneTest.cxx"
	"RandomTest.cxx"
	"RayTest.cxx"
	"SpriteClipTest.cxx"
	"TrackTest.cxx"
	"WeatherTest.cxx")

target_link_libraries(v3dtest_type PRIVATE v3dlib_type)
