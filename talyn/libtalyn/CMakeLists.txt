add_library("v3dlib_talyn"
	"RenderContext.h" "RenderContext.cxx"
	"RIBHandler.h" "RIBHandler.cxx"
)

# PUBLIC: the framebuffer and the shared scene are part of the render context's interface.
target_link_libraries(v3dlib_talyn PUBLIC v3dlib_render_offline v3dlib_type)

target_compile_options(v3dlib_talyn PRIVATE /EHsc /utf-8)
