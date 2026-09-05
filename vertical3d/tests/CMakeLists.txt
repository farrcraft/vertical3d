v3d_add_test(vertical3d
	"TestMain.cxx"

	"view/ViewLayoutTest.cxx"
	"view/CameraProfilesTest.cxx"
	"view/ConstructionPlaneTest.cxx"

	"scene/SceneTest.cxx"
	"scene/CreatePolyTest.cxx"
	"scene/WireframeVisitorTest.cxx"
	"scene/RIBExportVisitorTest.cxx"
	"scene/ProjectTest.cxx"

	"command/CommandDirectoryTest.cxx"
	"command/CommandStackTest.cxx"

	"tool/PickerTest.cxx"
	"tool/SelectToolTest.cxx"
	"tool/TransformToolTest.cxx"

	"manipulator/ManipulatorTest.cxx"

	"../src/view/ViewLayout.cxx"
	"../src/view/ViewPort.cxx"
	"../src/view/CameraProfiles.cxx"
	"../src/view/ConstructionPlane.cxx"
	"../src/scene/Scene.cxx"
	"../src/scene/MeshTopology.cxx"
	"../src/scene/CreatePoly.cxx"
	"../src/scene/WireframeVisitor.cxx"
	"../src/scene/RIBExportVisitor.cxx"
	"../src/scene/Project.cxx"
	"../src/command/CommandDirectory.cxx"
	"../src/command/CommandStack.cxx"
	"../src/command/CreateCommand.cxx"
	"../src/command/TransformCommand.cxx"
	"../src/tool/SelectMask.cxx"
	"../src/tool/SelectTool.cxx"
	"../src/tool/TransformTool.cxx"
	"../src/tool/Picker.cxx"
	"../src/manipulator/Manipulator.cxx"
	"../src/manipulator/TranslateManipulator.cxx"
	"../src/manipulator/RotateManipulator.cxx"
	"../src/manipulator/ScaleManipulator.cxx")

target_link_libraries(v3dtest_vertical3d PRIVATE
	v3dlib_asset
	v3dlib_brep
	v3dlib_event
	v3dlib_font
	v3dlib_log
	v3dlib_render
	v3dlib_render_offline
	v3dlib_type
	${Boost_LIBRARIES}
	${SDL3_LIBRARIES}
	${Vulkan_LIBRARIES}
	${PNG_LIBRARIES}
	JPEG::JPEG
	Freetype::Freetype)
