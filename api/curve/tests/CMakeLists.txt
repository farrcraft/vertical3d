# Curves are plain values. The tests need no window, no device, and no files copied beside the
# executable.
v3d_add_test(curve
	"TestMain.cxx"
	"BasisTest.cxx"
	"CurveTest.cxx"
	"CurvesTest.cxx")

target_link_libraries(v3dtest_curve PRIVATE v3dlib_curve)
