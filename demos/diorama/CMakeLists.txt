add_executable(diorama
	"src/Diorama.h" "src/Diorama.cxx"
	"src/Layout.h" "src/Layout.cxx"
	"src/Renderer.h" "src/Renderer.cxx"
	"src/main.cxx"
)

target_link_libraries(diorama PRIVATE
	v3dlib_asset
	v3dlib_audio
	v3dlib_ecs
	v3dlib_engine
	v3dlib_input
	v3dlib_log
	v3dlib_render
	v3dlib_shell
	v3dlib_type
	v3dlib_ui
	v3dlib_window
)

v3d_add_app_data(diorama)
v3d_add_shared_data(diorama)

# build for the windows subsystem. A console subsystem executable opens a console beside the
# app's window. SDL_main.h supplies the WinMain that calls main.
set_target_properties(diorama PROPERTIES WIN32_EXECUTABLE TRUE)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
