add_library("v3dlib_render"
	"realtime/2D/Context2D.h" "realtime/2D/Context2D.cpp"
	"realtime/Surface.h" "realtime/Surface.cpp"
	"realtime/Frame.h" "realtime/Frame.cpp"
	"realtime/Operation.h"
	"realtime/2D/Texture2D.h" "realtime/2D/Texture2D.cpp"
	"realtime/operation/2D/Blit2DTexture.h" "realtime/operation/2D/Blit2DTexture.cpp"
	"realtime/2D/Texture2DCache.h" "realtime/2D/Texture2DCache.cpp"
	"realtime/operation/GLTexturedQuad.h" "realtime/operation/GLTexturedQuad.cpp"
	"realtime/operation/GLTexture.h" "realtime/operation/GLTexture.cpp"
	"realtime/operation/Overlay.h" "realtime/operation/Overlay.cpp"
	"realtime/operation/GLFont.h" "realtime/operation/GLFont.cpp"
	"realtime/operation/TextureFont.h" "realtime/operation/TextureFont.cpp"
	"realtime/operation/BitmapFont.h" "realtime/operation/BitmapFont.cpp" 
	"realtime/2D/Engine2D.h" "realtime/2D/Engine2D.cpp" 
	"realtime/2D/Window2D.h" "realtime/2D/Window2D.cpp"
	"realtime/2D/Scene2D.h" "realtime/2D/Scene2D.cpp" 
	"realtime/Engine.h" "realtime/Engine.cpp" 
	"realtime/Context.h" "realtime/Window.h" "realtime/Window.cpp" 
	"realtime/2D/Operation2D.h" "realtime/2D/Operation2D.cpp" "realtime/Window3D.h" "realtime/Window3D.cpp" "realtime/Engine3D.h" "realtime/Engine3D.cpp" "realtime/operation/Canvas.h" "realtime/operation/Canvas.cpp")

# Workaround for a boost log ABI bug - https://github.com/microsoft/vcpkg/discussions/22762
target_compile_definitions(v3dlib_render PUBLIC BOOST_USE_WINAPI_VERSION=0x0A00)
target_compile_definitions(v3dlib_render PUBLIC _WIN32_WINNT=0x0A00)
target_compile_definitions(v3dlib_render PUBLIC WINVER=0x0A00)
