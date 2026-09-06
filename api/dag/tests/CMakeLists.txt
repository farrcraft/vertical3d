v3d_add_test(dag
	"TestMain.cxx"
	"NodeTest.cxx"
	"TransformTest.cxx")

target_link_libraries(v3dtest_dag PRIVATE v3dlib_dag)
