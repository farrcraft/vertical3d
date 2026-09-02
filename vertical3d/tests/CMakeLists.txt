# The layout, the camera profile table, the construction grid, the scene and the wireframe
# it is drawn as all need neither a window nor a device: the layout is arithmetic over a
# json tree, the profile table is a load, and the rest produce a LineCanvas, which is the
# cpu half of the line primitive of ADR-0011. The renderer is not testable this way and is
# not covered here.
v3d_add_test(vertical3d
	"TestMain.cxx"
	"CameraProfilesTest.cxx"
	"ConstructionPlaneTest.cxx"
	"CreatePolyTest.cxx"
	"SceneTest.cxx"
	"ViewLayoutTest.cxx"
	"WireframeVisitorTest.cxx"
	"../src/CameraProfiles.cxx"
	"../src/ConstructionPlane.cxx"
	"../src/CreatePoly.cxx"
	"../src/Scene.cxx"
	"../src/ViewLayout.cxx"
	"../src/WireframeVisitor.cxx")

target_link_libraries(v3dtest_vertical3d PRIVATE
	v3dlib_asset
	v3dlib_brep
	v3dlib_font
	v3dlib_log
	v3dlib_render
	v3dlib_type
	${Boost_LIBRARIES}
	${SDL3_LIBRARIES}
	${Vulkan_LIBRARIES}
	${PNG_LIBRARIES}
	JPEG::JPEG
	Freetype::Freetype)
