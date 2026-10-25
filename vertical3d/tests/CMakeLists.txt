# The controller is built so its commands can be checked against the bindings the editor
# ships, and is never started.
v3d_add_test(vertical3d
	"TestMain.cxx"
	"BindingsTest.cxx"

	"view/ViewLayoutTest.cxx"
	"view/ConstructionPlaneTest.cxx"
	"view/TriangulationTest.cxx"
	"view/ViewPortTest.cxx"

	"scene/ItemTest.cxx"
	"scene/SceneTest.cxx"
	"scene/EvaluationTest.cxx"
	"scene/PropertiesTest.cxx"
	"scene/CreateItemTest.cxx"
	"scene/CreatePolyTest.cxx"
	"scene/GlyphsTest.cxx"
	"scene/WireframeVisitorTest.cxx"
	"scene/RIBExportVisitorTest.cxx"
	"scene/ProjectTest.cxx"
	"scene/SceneStoreTest.cxx"
	"scene/ModellingTest.cxx"

	"command/CommandStackTest.cxx"
	"command/GroupCommandTest.cxx"
	"command/KeyCommandTest.cxx"
	"command/SettingsCommandTest.cxx"

	"render/DelegateTest.cxx"
	"render/BatchTest.cxx"
	"render/RenderPanelsTest.cxx"

	"tool/CameraControlToolTest.cxx"
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
	"../src/view/Triangulation.cxx"
	"../src/view/ShadedPass.cxx"
	"../src/scene/Item.cxx"
	"../src/scene/Mesh.cxx"
	"../src/scene/Camera.cxx"
	"../src/scene/Light.cxx"
	"../src/scene/Group.cxx"
	"../src/scene/Evaluation.cxx"
	"../src/scene/Properties.cxx"
	"../src/scene/Scene.cxx"
	"../src/scene/CreateItem.cxx"
	"../src/scene/CreatePoly.cxx"
	"../src/scene/Glyphs.cxx"
	"../src/scene/WireframeVisitor.cxx"
	"../src/scene/RIBExportVisitor.cxx"
	"../src/scene/Project.cxx"
	"../src/scene/SceneStore.cxx"
	"../src/scene/Modelling.cxx"
	"../src/render/Batch.cxx"
	"../src/render/Delegate.cxx"
	"../src/render/MoyaDelegate.cxx"
	"../src/render/Process.cxx"
	"../src/render/RenderPanels.cxx"
	"../src/render/Shot.cxx"
	"../src/command/CommandStack.cxx"
	"../src/command/CreateCommand.cxx"
	"../src/command/GroupCommand.cxx"
	"../src/command/KeyCommand.cxx"
	"../src/command/SettingsCommand.cxx"
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
	v3dlib_database
	v3dlib_engine
	v3dlib_event
	v3dlib_image
	v3dlib_log
	v3dlib_moya
	v3dlib_render
	v3dlib_render_offline
	v3dlib_type
	v3dlib_ui)

# the batch cases run moya.exe where the build puts it, beside the libraries it loads
add_dependencies(v3dtest_vertical3d moya)
target_compile_definitions(v3dtest_vertical3d PRIVATE V3D_MOYA="$<TARGET_FILE:moya>")

v3d_copy_data(v3dtest_vertical3d app "${CMAKE_CURRENT_SOURCE_DIR}/../data")

# the controller's renderer builds the shaded pass, whose stages are compiled in
v3d_add_shader(v3dtest_vertical3d "../shaders/shaded.vert")
v3d_add_shader(v3dtest_vertical3d "../shaders/shaded.frag")

# The cases that draw on a device: the shaded pass, through the render device suite's headless
# device. Where there is no device the binary exits with 77, which ctest reports as a skip.
v3d_add_test(vertical3d_device
	"device/TestMain.cxx"
	"device/ShadedPassTest.cxx"

	"../../api/render/tests/device/Headless.cpp"
	"../src/view/ShadedPass.cxx"
	"../src/view/Triangulation.cxx"
	"../src/scene/Item.cxx"
	"../src/scene/Mesh.cxx"
	"../src/scene/Camera.cxx"
	"../src/scene/Light.cxx"
	"../src/scene/Group.cxx"
	"../src/scene/Evaluation.cxx"
	"../src/scene/Properties.cxx"
	"../src/scene/Scene.cxx"
	"../src/scene/CreatePoly.cxx")

v3d_add_shader(v3dtest_vertical3d_device "../shaders/shaded.vert")
v3d_add_shader(v3dtest_vertical3d_device "../shaders/shaded.frag")

target_link_libraries(v3dtest_vertical3d_device PRIVATE
	v3dlib_brep
	v3dlib_dag
	v3dlib_image
	v3dlib_log
	v3dlib_render
	v3dlib_type)
set_tests_properties(vertical3d_device PROPERTIES SKIP_RETURN_CODE 77)
