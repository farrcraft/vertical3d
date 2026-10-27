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

	"document/DocumentTest.cxx"

	"control/PlaybackTest.cxx"
	"control/PointerTest.cxx"

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
	"tool/ToolRouterTest.cxx"
	"tool/TransformToolTest.cxx"

	"manipulator/ManipulatorTest.cxx"
	"manipulator/TargetTest.cxx")

target_link_libraries(v3dtest_vertical3d PRIVATE
	v3dlib_editor
	v3dlib_animation
	v3dlib_asset
	v3dlib_brep
	v3dlib_database
	v3dlib_engine
	v3dlib_event
	v3dlib_image
	v3dlib_log
	v3dlib_render
	v3dlib_render_offline
	v3dlib_type
	v3dlib_ui)

# the batch cases run moya.exe where the build puts it, beside the libraries it loads
add_dependencies(v3dtest_vertical3d moya)
target_compile_definitions(v3dtest_vertical3d PRIVATE V3D_MOYA="$<TARGET_FILE:moya>")

v3d_copy_data(v3dtest_vertical3d app "${CMAKE_CURRENT_SOURCE_DIR}/../data")

# The cases that draw on a device: the shaded pass, through the render device suite's headless
# device. Where there is no device the binary exits with 77, which ctest reports as a skip.
v3d_add_test(vertical3d_device
	"device/TestMain.cxx"
	"device/ShadedPassTest.cxx"

	"../../api/render/tests/device/Headless.cpp")

target_link_libraries(v3dtest_vertical3d_device PRIVATE
	v3dlib_editor
	v3dlib_brep
	v3dlib_image
	v3dlib_log
	v3dlib_render
	v3dlib_type)
set_tests_properties(vertical3d_device PROPERTIES SKIP_RETURN_CODE 77)
