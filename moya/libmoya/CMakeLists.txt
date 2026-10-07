add_library("v3dlib_moya" 
	"Bucket.h" "Bucket.cxx"
	"Footprint.h" "Footprint.cxx"
	"FrameBuffer.h" "FrameBuffer.cxx"
	"GridShader.h" "GridShader.cxx"
	"MicroPolygon.h" "MicroPolygon.cxx"
	"MicroPolygonGrid.h" "MicroPolygonGrid.cxx"
	"Polygon.h" "Polygon.cxx"
	"Hider.h" "Hider.cxx"
	"RayHider.h" "RayHider.cxx"
	"RenderContext.h" "RenderContext.cxx"
	"RIBHandler.h" "RIBHandler.cxx"
	"Renderer.h" "Renderer.cxx"
	"RenderMan.h" "RenderMan.cxx"
	"ReyesPrimitive.h" "ReyesPrimitive.cxx"
	"Samples.h" "Samples.cxx"
	"Shading.h"
	"Vertex.h" "Vertex.cxx"
)

# RenderMan.cxx declares the whole RenderMan Interface and leaves most of its entry points
# empty, so every parameter of every unimplemented one is unreferenced. That is 500-odd C4100
# reports at /W4 - enough to bury the rest of the tree's warnings - and naming the parameters
# out would only have to be undone as each function grows a body.
target_compile_options("v3dlib_moya" PRIVATE /wd4100 /EHsc /utf-8)

# PUBLIC: the framebuffer is part of the render context's interface, and the render context's
# header includes the log's. Its headers name glm types. PRIVATE: only the render context's source
# writes images.
target_link_libraries("v3dlib_moya" PUBLIC v3dlib_log v3dlib_render_offline v3dlib_type glm::glm)
target_link_libraries("v3dlib_moya" PRIVATE v3dlib_image)
