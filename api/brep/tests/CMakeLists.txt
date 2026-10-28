v3d_add_test(brep
	"TestMain.cxx"
	"BRepTest.cxx"
	"BuilderTest.cxx"
	"OperationsTest.cxx"
	"PrimitivesTest.cxx"
	"TriangulationTest.cxx"
	"FaceTest.cxx"
	"HalfEdgeTest.cxx"
	"VertexTest.cxx")

target_link_libraries(v3dtest_brep PRIVATE v3dlib_brep v3dlib_type)
