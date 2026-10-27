add_executable(odyssey
	"src/engine/Engine.h" "src/engine/Engine.cpp"
	"src/engine/Path.h"
	"src/engine/Player.h" "src/engine/Player.cpp"
	"src/engine/Unit.h"
	"src/render/Renderer.h" "src/render/Renderer.cpp"
	"src/system/Movement.h" "src/system/Movement.cpp"
	"src/tile/Map.h" "src/tile/Map.cpp"
	"src/tile/Size.h"
	"src/tile/Tile.h"
	"src/tile/World.h" "src/tile/World.cpp"
	"src/odyssey.cpp"
)

target_link_libraries(odyssey PRIVATE
	v3dlib_asset
	v3dlib_asset_media
	v3dlib_ecs
	v3dlib_engine
	v3dlib_event
	v3dlib_grid
	v3dlib_input
	v3dlib_log
	v3dlib_render
	v3dlib_shell
	v3dlib_type
	v3dlib_ui
	v3dlib_window
)

v3d_add_app_data(odyssey)
v3d_add_shared_data(odyssey)

# build for the windows subsystem. A console subsystem executable opens a console beside the
# app's window. SDL_main.h supplies the WinMain that calls main.
set_target_properties(odyssey PROPERTIES WIN32_EXECUTABLE TRUE)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
