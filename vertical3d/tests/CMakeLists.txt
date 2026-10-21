# The controller is built so its commands can be checked against the bindings the editor
# ships, and is never started.
v3d_add_test(vertical3d
	"TestMain.cxx"
	"BindingsTest.cxx"

	"view/ViewLayoutTest.cxx"
	"view/ConstructionPlaneTest.cxx"

	"scene/SceneTest.cxx"
	"scene/CreatePolyTest.cxx"
	"scene/WireframeVisitorTest.cxx"
	"scene/RIBExportVisitorTest.cxx"
	"scene/ProjectTest.cxx"
	"scene/ModellingTest.cxx"

	"command/CommandStackTest.cxx"

	"tool/PickerTest.cxx"
	"tool/SelectToolTest.cxx"
	"tool/TransformToolTest.cxx"

	"manipulator/ManipulatorTest.cxx"
	"manipulator/TargetTest.cxx"

	"../src/Controller.cxx"
	"../src/Renderer.cxx"
	"../src/view/ViewLayout.cxx"
	"../src/view/ViewPort.cxx"
	"../src/view/ConstructionPlane.cxx"
	"../src/scene/Scene.cxx"
	"../src/scene/CreatePoly.cxx"
	"../src/scene/WireframeVisitor.cxx"
	"../src/scene/RIBExportVisitor.cxx"
	"../src/scene/Project.cxx"
	"../src/scene/Modelling.cxx"
	"../src/command/CommandStack.cxx"
	"../src/command/CreateCommand.cxx"
	"../src/command/Placement.cxx"
	"../src/command/TransformCommand.cxx"
	"../src/command/VertexCommand.cxx"
	"../src/command/TopologyCommand.cxx"
	"../src/tool/CameraControlTool.cxx"
	"../src/tool/SelectMask.cxx"
	"../src/tool/SelectTool.cxx"
	"../src/tool/TransformTool.cxx"
	"../src/tool/Picker.cxx"
	"../src/manipulator/Manipulator.cxx"
	"../src/manipulator/Target.cxx"
	"../src/manipulator/TranslateManipulator.cxx"
	"../src/manipulator/RotateManipulator.cxx"
	"../src/manipulator/ScaleManipulator.cxx")

target_link_libraries(v3dtest_vertical3d PRIVATE
	v3dlib_asset
	v3dlib_asset_media
	v3dlib_brep
	v3dlib_config
	v3dlib_dag
	v3dlib_engine
	v3dlib_event
	v3dlib_log
	v3dlib_render
	v3dlib_render_offline
	v3dlib_type
	v3dlib_ui)

v3d_copy_data(v3dtest_vertical3d app "${CMAKE_CURRENT_SOURCE_DIR}/../data")
