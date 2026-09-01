# CMakeList.txt : CMake project for Odyssey, include source and define
# project specific logic here.
#

# Boost comes from the root CMakeLists - asking for it again here would narrow
# Boost_LIBRARIES to just the components named, and odyssey needs json through v3dlib_asset.

link_directories(
	${Boost_LIBRARY_DIRS}
	"../vendor/libjpeg-turbo-2.1.0"
	"../vendor/libpng-1.6.37"
	"../vendor/zlib-1.2.11")

# Add source to this project's executable.
add_executable (Odyssey WIN32 "Odyssey.cpp"  "ui/Screen.h" "ui/Screen.cpp"
 "engine/Engine.h" "engine/Engine.cpp"
 "engine/Sprite.h" "engine/Player.h" "engine/SpriteSheet.h" 
  "render/Renderable.h" "engine/Player.cpp" 
"render/renderable/Player.h" "render/renderable/Player.cpp" 
"render/Renderable.cpp" 
 "actor/Actor.h"
 "render/Scene.h" "render/Scene.cpp" 
"system/Movement.h" "system/Movement.cpp"
"tile/Tile.h" "tile/Size.h" "engine/Unit.h"
)

# SDL3MAIN_LIBRARY is also specified within SDL3_LIBRARY by the FindSDL3 module.
# It's needed for Windows specific main function.
# If I don't use FindSDL3 module, I have to link it as well.
target_link_libraries(Odyssey
#		${SDL3MAIN_LIBRARY}
        ${SDL3_LIBRARIES}
		${Vulkan_LIBRARIES}
		${Boost_LIBRARIES}
		JPEG::JPEG
		${PNG_LIBRARIES}
#		"turbojpeg-static"
#		"jpeg-static"
#		"libpng16d"
#		"zlibd"
		"v3dlib_asset"
		"v3dlib_config"
		"v3dlib_ecs"
		"v3dlib_engine"
		"v3dlib_event"
		"v3dlib_font"
		"v3dlib_gl"
		"v3dlib_input"
		"v3dlib_log"
		"v3dlib_image"
		"v3dlib_type"
		"v3dlib_ui"
		"v3dlib_render"
		Freetype::Freetype
)

target_compile_options(Odyssey PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")
