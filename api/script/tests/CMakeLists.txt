# Scripts are text run in memory. The tests need no window, no device, and no files copied
# beside the executable.
v3d_add_test(script
	"TestMain.cxx"
	"RuntimeTest.cxx"
	"StripTest.cxx")

target_link_libraries(v3dtest_script PRIVATE v3dlib_script)

