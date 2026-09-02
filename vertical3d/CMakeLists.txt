add_executable("vertical3d"
	"src/CameraControlTool.h" "src/CameraControlTool.cxx"
	"src/CameraProfiles.h" "src/CameraProfiles.cxx"
	"src/ConstructionPlane.h" "src/ConstructionPlane.cxx"
	"src/Controller.h" "src/Controller.cxx"
	"src/CreatePoly.h" "src/CreatePoly.cxx"
	"src/MeshTopology.h" "src/MeshTopology.cxx"
	"src/Picker.h" "src/Picker.cxx"
	"src/Renderer.h" "src/Renderer.cxx"
	"src/Scene.h" "src/Scene.cxx"
	"src/SceneVisitor.h"
	"src/SelectMask.h" "src/SelectMask.cxx"
	"src/SelectTool.h" "src/SelectTool.cxx"
	"src/Tool.h"
	"src/ViewLayout.h" "src/ViewLayout.cxx"
	"src/ViewPort.h" "src/ViewPort.cxx"
	"src/WireframeVisitor.h" "src/WireframeVisitor.cxx"
	"src/main.cxx")

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
	${Boost_LIBRARIES}
	${SDL3_LIBRARIES}
	${Vulkan_LIBRARIES}
	${PNG_LIBRARIES}
	JPEG::JPEG
	Freetype::Freetype
)

v3d_add_app_data(vertical3d)

target_compile_options(vertical3d PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")

add_subdirectory("tests")
