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
	"ui/Screen.h" "ui/Screen.cpp"
)

# SDL3MAIN_LIBRARY is also specified within SDL3_LIBRARY by the FindSDL3 module.
# It's needed for Windows specific main function.
# If I don't use FindSDL3 module, I have to link it as well.
target_link_libraries(Odyssey
		"v3dlib_asset"
		"v3dlib_config"
		"v3dlib_ecs"
		"v3dlib_engine"
		"v3dlib_event"
		"v3dlib_font"
		"v3dlib_grid"
		"v3dlib_input"
		"v3dlib_log"
		"v3dlib_image"
		"v3dlib_type"
		"v3dlib_ui"
		"v3dlib_render"
)

v3d_add_app_data(Odyssey)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()

# a console subsystem executable is given a console of its own alongside the app's window.
# main is reached through the WinMain that SDL_main.h supplies.
set_target_properties(Odyssey PROPERTIES WIN32_EXECUTABLE TRUE)

target_compile_options(Odyssey PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")
