# The layout, the camera profile table, the construction grid, the scene, the wireframe it is
# drawn as, picking, the manipulators and the history all need neither a window nor a device:
# the layout is arithmetic over a json tree, the profile table is a load, picking is a ray
# cast against the cpu side of a mesh per ADR-0014, a manipulator is arithmetic over a camera
# and a transform, the history is a pair of stacks per ADR-0016, and the rest produce a
# LineCanvas, which is the cpu half of the line primitive of ADR-0011. The renderer is not
# testable this way and is not covered here.
v3d_add_test(vertical3d
	"TestMain.cxx"
	"CameraProfilesTest.cxx"
	"CommandStackTest.cxx"
	"ConstructionPlaneTest.cxx"
	"CreatePolyTest.cxx"
	"ManipulatorTest.cxx"
	"PickerTest.cxx"
	"SceneTest.cxx"
	"SelectToolTest.cxx"
	"TransformToolTest.cxx"
	"ViewLayoutTest.cxx"
	"WireframeVisitorTest.cxx"
	"../src/CameraProfiles.cxx"
	"../src/CommandStack.cxx"
	"../src/ConstructionPlane.cxx"
	"../src/CreateCommand.cxx"
	"../src/CreatePoly.cxx"
	"../src/Manipulator.cxx"
	"../src/MeshTopology.cxx"
	"../src/Picker.cxx"
	"../src/RotateManipulator.cxx"
	"../src/ScaleManipulator.cxx"
	"../src/Scene.cxx"
	"../src/SelectMask.cxx"
	"../src/SelectTool.cxx"
	"../src/TransformCommand.cxx"
	"../src/TransformTool.cxx"
	"../src/TranslateManipulator.cxx"
	"../src/ViewPort.cxx"
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
