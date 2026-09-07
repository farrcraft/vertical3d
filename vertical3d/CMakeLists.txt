add_executable("vertical3d"
	"src/Controller.h" "src/Controller.cxx"
	"src/Renderer.h" "src/Renderer.cxx"
	"src/main.cxx"

	"src/view/ViewLayout.h" "src/view/ViewLayout.cxx"
	"src/view/ViewPort.h" "src/view/ViewPort.cxx"
	"src/view/ConstructionPlane.h" "src/view/ConstructionPlane.cxx"

	"src/scene/Scene.h" "src/scene/Scene.cxx"
	"src/scene/SceneVisitor.h"
	"src/scene/MeshTopology.h" "src/scene/MeshTopology.cxx"
	"src/scene/CreatePoly.h" "src/scene/CreatePoly.cxx"
	"src/scene/WireframeVisitor.h" "src/scene/WireframeVisitor.cxx"
	"src/scene/RIBExportVisitor.h" "src/scene/RIBExportVisitor.cxx"
	"src/scene/Project.h" "src/scene/Project.cxx"

	"src/command/Command.h"
	"src/command/CommandDirectory.h" "src/command/CommandDirectory.cxx"
	"src/command/CommandStack.h" "src/command/CommandStack.cxx"
	"src/command/CreateCommand.h" "src/command/CreateCommand.cxx"
	"src/command/TransformCommand.h" "src/command/TransformCommand.cxx"

	"src/tool/Tool.h"
	"src/tool/CameraControlTool.h" "src/tool/CameraControlTool.cxx"
	"src/tool/SelectMask.h" "src/tool/SelectMask.cxx"
	"src/tool/SelectTool.h" "src/tool/SelectTool.cxx"
	"src/tool/TransformTool.h" "src/tool/TransformTool.cxx"
	"src/tool/Picker.h" "src/tool/Picker.cxx"

	"src/manipulator/Manipulator.h" "src/manipulator/Manipulator.cxx"
	"src/manipulator/TranslateManipulator.h" "src/manipulator/TranslateManipulator.cxx"
	"src/manipulator/RotateManipulator.h" "src/manipulator/RotateManipulator.cxx"
	"src/manipulator/ScaleManipulator.h" "src/manipulator/ScaleManipulator.cxx")

target_link_libraries(vertical3d PRIVATE
	v3dlib_asset
	v3dlib_brep
	v3dlib_config
	v3dlib_ecs
	v3dlib_engine
	v3dlib_event
	v3dlib_font
	v3dlib_image
	v3dlib_input
	v3dlib_log
	v3dlib_render
	v3dlib_type
	v3dlib_ui
)

v3d_add_shared_data(vertical3d)
v3d_add_app_data(vertical3d)

# a console subsystem executable is given a console of its own alongside the app's window.
# main is reached through the WinMain that SDL_main.h supplies.
set_target_properties(vertical3d PROPERTIES WIN32_EXECUTABLE TRUE)

target_compile_options(vertical3d PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
