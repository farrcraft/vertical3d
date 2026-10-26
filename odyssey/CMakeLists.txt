# CMakeList.txt : CMake project for Odyssey, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable(Odyssey
	"Odyssey.cpp"
	"actor/Actor.h"
	"engine/Engine.h" "engine/Engine.cpp"
	"engine/Path.h"
	"engine/Player.h" "engine/Player.cpp"
	"engine/Sprite.h" "engine/SpriteSheet.h" "engine/Unit.h"
	"render/Renderer.h" "render/Renderer.cpp"
	"system/Movement.h" "system/Movement.cpp"
	"tile/Map.h" "tile/Map.cpp"
	"tile/Tile.h" "tile/Size.h"
	"tile/World.h" "tile/World.cpp"
	"ui/Screen.h" "ui/Screen.cpp"
)

target_link_libraries(Odyssey
		"v3dlib_asset"
		"v3dlib_asset_media"
		"v3dlib_config"
		"v3dlib_ecs"
		"v3dlib_engine"
		"v3dlib_event"
		"v3dlib_grid"
		"v3dlib_input"
		"v3dlib_log"
		"v3dlib_render"
		"v3dlib_type"
		"v3dlib_window"
)

v3d_add_app_data(Odyssey)
v3d_add_shared_data(Odyssey)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()

# build for the windows subsystem. A console subsystem executable opens a console beside the
# app's window. SDL_main.h supplies the WinMain that calls main.
set_target_properties(Odyssey PROPERTIES WIN32_EXECUTABLE TRUE)

