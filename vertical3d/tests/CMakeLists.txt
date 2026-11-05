# The controller is built so its commands can be checked against the bindings the editor
# ships, and is never started.
v3d_add_test(vertical3d
	"TestMain.cxx"
	"BindingsTest.cxx"

	"view/AreaLayoutTest.cxx"
	"view/AreaViewsTest.cxx"
	"view/AreaFrameTest.cxx"
	"view/ViewOverlayTest.cxx"
	"view/ConstructionPlaneTest.cxx"
	"view/ViewPortTest.cxx"
	"view/ThroughTest.cxx"

	"scene/ItemTest.cxx"
	"scene/ComponentsTest.cxx"
	"scene/CurvePartsTest.cxx"
	"scene/SceneTest.cxx"
	"scene/EvaluationTest.cxx"
	"scene/PropertiesTest.cxx"
	"scene/CreateItemTest.cxx"
	"scene/ImportTest.cxx"
	"script/ArgumentsTest.cxx"
	"script/TextTest.cxx"
	"script/DeclarationsTest.cxx"
	"view/GlyphsTest.cxx"
	"view/WireframeVisitorTest.cxx"
	"render/RIBExportVisitorTest.cxx"
	"store/ProjectTest.cxx"
	"store/CodecTest.cxx"
	"store/SceneTest.cxx"
	"command/ModellingTest.cxx"

	"document/DocumentTest.cxx"

	"control/PlaybackTest.cxx"
	"control/PointerTest.cxx"
	"control/AreaChromeTest.cxx"
	"control/CommandSearchTest.cxx"
	"control/IconSheetTest.cxx"
	"control/DividerHintTest.cxx"
	"control/NoticesTest.cxx"
	"control/OutlinerTest.cxx"
	"control/PropertiesEditorTest.cxx"
	"control/TimelineEditorTest.cxx"
	"control/GraphEditorTest.cxx"
	"control/ConsoleEditorTest.cxx"
	"app/CommandTableTest.cxx"
	"app/ScriptingTest.cxx"
	"app/ContextTest.cxx"
	"app/WorkspaceSwitcherTest.cxx"
	"control/RenderSettingsEditorTest.cxx"
	"control/RenderViewTest.cxx"
	"control/StatusBarTest.cxx"
	"control/TopBarTest.cxx"
	"control/WorkspacesTest.cxx"

	"command/StackTest.cxx"
	"command/GroupTest.cxx"
	"command/ParentTest.cxx"
	"command/ShowTest.cxx"
	"command/KeyTest.cxx"
	"command/KeyedTest.cxx"
	"command/KeysTest.cxx"
	"command/TimelineTest.cxx"
	"command/SettingsTest.cxx"
	"command/UnitsTest.cxx"

	"render/DelegateTest.cxx"
	"render/BatchTest.cxx"

	"tool/CameraControlTest.cxx"
	"tool/CameraLockTest.cxx"
	"tool/PickerTest.cxx"
	"tool/SelectTest.cxx"
	"tool/RouterTest.cxx"
	"tool/TransformTest.cxx"

	"manipulator/ManipulatorTest.cxx"
	"manipulator/TargetTest.cxx")

target_link_libraries(v3dtest_vertical3d PRIVATE
	v3dlib_editor
	v3dlib_animation
	v3dlib_asset
	v3dlib_brep
	v3dlib_curve
	v3dlib_database
	v3dlib_engine
	v3dlib_event
	v3dlib_image
	v3dlib_interchange
	v3dlib_log
	v3dlib_render
	v3dlib_render_offline
	v3dlib_type
	v3dlib_ui)

# the batch cases run moya.exe where the build puts it, beside the libraries it loads
add_dependencies(v3dtest_vertical3d moya)
target_compile_definitions(v3dtest_vertical3d PRIVATE V3D_MOYA="$<TARGET_FILE:moya>")

# the import cases read the interchange suite's fixtures where they are kept
target_compile_definitions(v3dtest_vertical3d PRIVATE V3D_INTERCHANGE_DATA="${V3D_ROOT}/api/interchange/tests/data")

v3d_copy_data(v3dtest_vertical3d app "${CMAKE_CURRENT_SOURCE_DIR}/../data")
v3d_copy_usd_plugins(v3dtest_vertical3d)

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

# The panels' cases measure through the ui suite's monospaced fixture face.
add_custom_command(TARGET v3dtest_vertical3d POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_SOURCE_DIR}/api/ui/tests/fonts"
		"$<TARGET_FILE_DIR:v3dtest_vertical3d>/fonts"
	COMMENT "Copying ui test fonts"
	VERBATIM)
