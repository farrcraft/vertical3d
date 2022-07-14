# CMakeList.txt : CMake project for Odyssey, include source and define
# project specific logic here.
#
set(CMAKE_VERBOSE_MAKEFILE ON)
cmake_minimum_required (VERSION 3.8)

# I haven't figured out how to make this work with a relative path
set(CMAKE_MODULE_PATH "C:/Users/sigsegv/source/repos/Odyssey/Odyssey/cmake")

list(APPEND CMAKE_PREFIX_PATH "C:/Users/sigsegv/source/repos/Odyssey/vendor/boost/boost_1_79_0")

# This path is used by FindSDL2 module. 
# It will find include and lib folders under this directory.
# This is because find command doesn't include any path for Windows.
# So it can't find anything using default paths.
# SDL2_PATH is already expected in module file. I'm using the same variable.
set(SDL2_PATH "../vendor/sdl2/SDL2-2.0.14")

# CMake parses module files by their names. It will use FindSDL2 in this case.
find_package(SDL2 REQUIRED)

# SDL2_INCLUDE_DIR variable set by the FindSDL2 module (if it finds directory).
include_directories(SYSTEM ${SDL2_INCLUDE_DIR})

# set(Boost_DEBUG ON)

# Need to set this for the COMPONENTS stanza to work next
set(Boost_USE_STATIC_LIBS ON)

# CMake already comes with a module for boost - https://cmake.org/cmake/help/latest/module/FindBoost.html
find_package(Boost 1.76.0 REQUIRED COMPONENTS log)

include_directories(SYSTEM
	${Boost_INCLUDE_DIRS}
	"../vendor/entt-3.10.1/src"
	"../vendor/libjpeg-turbo-2.1.0"
	"../vendor/libpng-1.6.37"
	"../vendor/zlib-1.2.11"
	"../vendor/glm-0.9.9.8/glm")
	
link_directories(
	${Boost_LIBRARY_DIRS}
	"../vendor/libjpeg-turbo-2.1.0"
	"../vendor/libpng-1.6.37"
	"../vendor/zlib-1.2.11")

# This gets us C++17 and newer language features, e.g. nested namespace support - https://docs.microsoft.com/en-us/cpp/error-messages/compiler-errors-1/compiler-error-c2429?view=msvc-160
#add_compile_options("/std:c++latest")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source to this project's executable.
add_executable (Odyssey WIN32 "Odyssey.cpp" "Odyssey.h" "ui/Window.h" 
"engine/Engine.h" "engine/Engine.cpp" "ui/Window.cpp" "ui/Screen.h" "ui/Screen.cpp" "config/Bootstrap.h" "config/Bootstrap.cpp" "config/JsonFile.h" "config/JsonFile.cpp" 
"asset/loader/Jpeg.cpp" "engine/Sprite.h" "render/Texture.h"  "render/Frame.h" "engine/Player.h" "engine/SpriteSheet.h" 
"render/Engine.h" "render/Engine.cpp" "render/Renderable.h" "engine/Player.cpp" "render/renderable/Player.h" "render/renderable/Player.cpp" "render/Renderable.cpp" 
"asset/Manager.h" "asset/Asset.h" "asset/Manager.cpp" "asset/Asset.cpp" "asset/Type.h"   "render/Texture.cpp" "asset/Loader.h" "asset/Cache.h" "render/TextureCache.h"
"render/Surface.h" "render/Surface.cpp" "actor/Actor.h" "render/TextureCache.cpp" "asset/loader/Png.h" "asset/loader/Png.cpp" "asset/Image.h" "asset/Image.cpp" "asset/Loader.cpp" 
"render/operation/BlitTexture.h" "render/Operation.h" "render/Frame.cpp" "render/operation/BlitTexture.cpp" "render/Context.h" "render/Context.cpp" "render/Scene.h" "render/Scene.cpp" 
"tile/Tile.h" "tile/Size.h" "engine/Unit.h" "component/Position.h" "component/Position.cpp" "input/Engine.h" "input/Engine.cpp" "input/Device.h" "input/Device.cpp" "input/Mouse.h" "input/Mouse.cpp" 
"input/Keyboard.h" "input/Keyboard.cpp" "event/KeyUp.h" "config/BindingContext.h" "system/Movement.h"  "input/KeyState.h" "input/KeyState.cpp" "asset/Json.h" "asset/Json.cpp" "asset/loader/Json.h" 
"asset/loader/Json.cpp" "system/Movement.cpp" "asset/Cache.cpp" "config/BindingContext.cpp" "config/Config.h" "config/Config.cpp" "event/Key.h" "event/Key.cpp" "event/KeyUp.cpp" "event/KeyDown.h" "event/KeyDown.cpp")

# SDL2MAIN_LIBRARY is also specified within SDL2_LIBRARY by the FindSDL2 module.
# It's needed for Windows specific main function.
# If I don't use FindSDL2 module, I have to link it as well.
target_link_libraries(Odyssey
		${SDL2MAIN_LIBRARY}
        ${SDL2_LIBRARY}
		${Boost_LIBRARIES}
		"turbojpeg-static" "jpeg-static"
		"libpng16d" "zlibd")
