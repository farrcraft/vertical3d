v3d_add_test(brep
	"TestMain.cxx"
	"BRepTest.cxx"
	"FaceTest.cxx"
	"HalfEdgeTest.cxx"
	"VertexTest.cxx")

target_link_libraries(v3dtest_brep PRIVATE v3dlib_brep v3dlib_type)
