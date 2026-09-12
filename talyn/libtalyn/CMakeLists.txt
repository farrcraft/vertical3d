add_library("v3dlib_talyn"
	"HitShader.h" "HitShader.cxx"
	"RenderContext.h" "RenderContext.cxx"
	"RIBHandler.h" "RIBHandler.cxx"
	"Scene.h" "Scene.cxx"
)

# PUBLIC: the framebuffer is part of the render context's interface, and the camera and the
# ray are part of the scene's.
target_link_libraries(v3dlib_talyn PUBLIC v3dlib_render_offline v3dlib_type)

target_compile_options(v3dlib_talyn PRIVATE /EHsc /utf-8)
