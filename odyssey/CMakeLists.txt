﻿# CMakeList.txt : CMake project for Odyssey, include source and define
# project specific logic here.
#

# CMake already comes with a module for boost - https://cmake.org/cmake/help/latest/module/FindBoost.html
find_package(Boost 1.76.0 REQUIRED COMPONENTS log)


link_directories(
	${Boost_LIBRARY_DIRS}
	"../vendor/libjpeg-turbo-2.1.0"
	"../vendor/libpng-1.6.37"
	"../vendor/zlib-1.2.11")

# Add source to this project's executable.
add_executable (Odyssey WIN32 "Odyssey.cpp" "Odyssey.h" "ui/Screen.h" "ui/Screen.cpp"
 "engine/Sprite.h" "render/Texture.h"  "render/Frame.h" "engine/Player.h" "engine/SpriteSheet.h" 
"render/Engine.h" "render/Engine.cpp" "render/Renderable.h" "engine/Player.cpp" 
"render/renderable/Player.h" "render/renderable/Player.cpp" 
"render/Renderable.cpp" 
 "render/Texture.cpp" "render/TextureCache.h"
"render/Surface.h" "render/Surface.cpp" "actor/Actor.h" "render/TextureCache.cpp" 
"render/operation/BlitTexture.h" "render/Operation.h" "render/Frame.cpp" "render/operation/BlitTexture.cpp" "render/Context.h" "render/Context.cpp" "render/Scene.h" "render/Scene.cpp" 
"tile/Tile.h" "tile/Size.h" "engine/Unit.h"
"config/BindingContext.h" "system/Movement.h" "system/Movement.cpp" "config/BindingContext.cpp" "config/Config.h" 
"config/Config.cpp"
)

# SDL2MAIN_LIBRARY is also specified within SDL2_LIBRARY by the FindSDL2 module.
# It's needed for Windows specific main function.
# If I don't use FindSDL2 module, I have to link it as well.
target_link_libraries(Odyssey
		${SDL2MAIN_LIBRARY}
        ${SDL2_LIBRARY}
		${Boost_LIBRARIES}
		"turbojpeg-static"
		"jpeg-static"
		"libpng16d"
		"zlibd"
		"v3dlib_asset"
		"v3dlib_component"
		"v3dlib_log"
		"v3dlib_image"
		"v3dlib_ui"
)
