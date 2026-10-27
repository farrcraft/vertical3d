# The editor's sources, built once, so the executable and its suites link one copy of them
# rather than each compiling their own.
add_library("v3dlib_editor"
	"src/Controller.h" "src/Controller.cxx"
	"src/Renderer.h" "src/Renderer.cxx"

	"src/document/Document.h" "src/document/Document.cxx"

	"src/control/Playback.h" "src/control/Playback.cxx"
	"src/control/Pointer.h" "src/control/Pointer.cxx"
	"src/control/RenderWindow.h" "src/control/RenderWindow.cxx"

	"src/view/ViewLayout.h" "src/view/ViewLayout.cxx"
	"src/view/ViewPort.h" "src/view/ViewPort.cxx"
	"src/view/ConstructionPlane.h" "src/view/ConstructionPlane.cxx"
	"src/view/Shading.h"
	"src/view/ShadedPass.h" "src/view/ShadedPass.cxx"

	"src/scene/Item.h" "src/scene/Item.cxx"
	"src/scene/Mesh.h" "src/scene/Mesh.cxx"
	"src/scene/Components.h" "src/scene/Components.cxx"
	"src/scene/Camera.h" "src/scene/Camera.cxx"
	"src/scene/Light.h" "src/scene/Light.cxx"
	"src/scene/Group.h" "src/scene/Group.cxx"
	"src/scene/Curve.h" "src/scene/Curve.cxx"
	"src/scene/CurveParts.h" "src/scene/CurveParts.cxx"
	"src/scene/Import.h" "src/scene/Import.cxx"
	"src/scene/Evaluation.h" "src/scene/Evaluation.cxx"
	"src/scene/Properties.h" "src/scene/Properties.cxx"
	"src/scene/RenderSettings.h"
	"src/scene/Timeline.h"
	"src/scene/Scene.h" "src/scene/Scene.cxx"
	"src/scene/SceneVisitor.h"
	"src/scene/CreateItem.h" "src/scene/CreateItem.cxx"
	"src/view/Glyphs.h" "src/view/Glyphs.cxx"
	"src/view/WireframeVisitor.h" "src/view/WireframeVisitor.cxx"
	"src/render/RIBExportVisitor.h" "src/render/RIBExportVisitor.cxx"
	"src/store/Project.h" "src/store/Project.cxx"
	"src/store/SceneStore.h" "src/store/SceneStore.cxx"
	"src/store/Bytes.h" "src/store/Bytes.cxx"
	"src/store/Payload.h" "src/store/Payload.cxx"
	"src/store/Codec.h" "src/store/Codec.cxx"
	"src/store/MeshCodec.h" "src/store/MeshCodec.cxx"
	"src/store/CameraCodec.h" "src/store/CameraCodec.cxx"
	"src/store/LightCodec.h" "src/store/LightCodec.cxx"
	"src/store/GroupCodec.h" "src/store/GroupCodec.cxx"
	"src/store/CurveCodec.h" "src/store/CurveCodec.cxx"
	"src/command/Modelling.h" "src/command/Modelling.cxx"

	"src/render/Batch.h" "src/render/Batch.cxx"
	"src/render/Delegate.h" "src/render/Delegate.cxx"
	"src/render/MoyaDelegate.h" "src/render/MoyaDelegate.cxx"
	"src/render/Process.h" "src/render/Process.cxx"
	"src/render/RenderPanels.h" "src/render/RenderPanels.cxx"
	"src/render/Shot.h" "src/render/Shot.cxx"

	"src/command/Command.h"
	"src/command/CommandStack.h" "src/command/CommandStack.cxx"
	"src/command/CreateCommand.h" "src/command/CreateCommand.cxx"
	"src/command/GroupCommand.h" "src/command/GroupCommand.cxx"
	"src/command/ImportCommand.h" "src/command/ImportCommand.cxx"
	"src/command/KeyCommand.h" "src/command/KeyCommand.cxx"
	"src/command/SettingsCommand.h" "src/command/SettingsCommand.cxx"
	"src/command/Placement.h" "src/command/Placement.cxx"
	"src/command/TransformCommand.h" "src/command/TransformCommand.cxx"
	"src/command/VertexCommand.h" "src/command/VertexCommand.cxx"
	"src/command/TopologyCommand.h" "src/command/TopologyCommand.cxx"

	"src/tool/Tool.h"
	"src/tool/ToolRouter.h" "src/tool/ToolRouter.cxx"
	"src/tool/CameraControlTool.h" "src/tool/CameraControlTool.cxx"
	"src/selection/SelectMask.h" "src/selection/SelectMask.cxx"
	"src/tool/SelectTool.h" "src/tool/SelectTool.cxx"
	"src/tool/TransformTool.h" "src/tool/TransformTool.cxx"
	"src/tool/Picker.h" "src/tool/Picker.cxx"

	"src/manipulator/Manipulator.h" "src/manipulator/Manipulator.cxx"
	"src/manipulator/Target.h" "src/manipulator/Target.cxx"
	"src/manipulator/TranslateManipulator.h" "src/manipulator/TranslateManipulator.cxx"
	"src/manipulator/RotateManipulator.h" "src/manipulator/RotateManipulator.cxx"
	"src/manipulator/ScaleManipulator.h" "src/manipulator/ScaleManipulator.cxx")

target_include_directories("v3dlib_editor" PUBLIC ${V3D_ROOT})
target_link_libraries("v3dlib_editor" PUBLIC
	v3dlib_animation
	v3dlib_asset
	v3dlib_brep
	v3dlib_curve
	v3dlib_engine
	v3dlib_event
	v3dlib_image
	v3dlib_interchange
	v3dlib_log
	v3dlib_render
	v3dlib_render_offline
	v3dlib_type
	v3dlib_shell
	v3dlib_ui
	v3dlib_window)

# PRIVATE: what only the editor's sources use. The renderer loads the ui's images, the project
# store writes through the database, the document imports USD, and moya renders the render
# window
target_link_libraries("v3dlib_editor" PRIVATE v3dlib_asset_media v3dlib_database v3dlib_interchange_usd v3dlib_moya)

# the shaded pass's own stages, which a view draws its meshes' surfaces with
v3d_add_shader("v3dlib_editor" "shaders/shaded.vert")
v3d_add_shader("v3dlib_editor" "shaders/shaded.frag")

add_executable("vertical3d" "src/main.cxx")
target_link_libraries(vertical3d PRIVATE v3dlib_editor v3dlib_engine)

# a batch render runs moya.exe, which the editor finds beside itself
v3d_copy_program(vertical3d moya)

# importing a USD stage reads OpenUSD's plugins from beside the editor
v3d_copy_usd_plugins(vertical3d)

v3d_add_shared_data(vertical3d)
v3d_add_app_data(vertical3d)

# a windows subsystem executable, so no console opens beside the app's window. It starts in
# the WinMain that SDL_main.h supplies, and WinMain calls main.
set_target_properties(vertical3d PROPERTIES WIN32_EXECUTABLE TRUE)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
