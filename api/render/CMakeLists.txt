add_library("v3dlib_render"
	"realtime/Context.h" "realtime/Context.cpp"
	"realtime/Surface.h" "realtime/Surface.cpp"
	"realtime/Frame.h" "realtime/Frame.cpp"
	"realtime/Operation.h"
	"realtime/Texture.h" "realtime/Texture.cpp"
	"realtime/operation/BlitTexture.h" "realtime/operation/BlitTexture.cpp"
	"realtime/TextureCache.h" "realtime/TextureCache.cpp" "realtime/operation/GLTexturedQuad.h" "realtime/operation/GLTexturedQuad.cpp"
	"realtime/operation/GLTexture.h" "realtime/operation/GLTexture.cpp" "realtime/operation/Overlay.h" "realtime/operation/Overlay.cpp" "realtime/operation/GLFont.h" "realtime/operation/GLFont.cpp")