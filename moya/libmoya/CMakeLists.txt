add_library("v3dlib_moya" 
	"reyes/Bucket.h" "reyes/Bucket.cxx"
	"reyes/Footprint.h" "reyes/Footprint.cxx"
	"FrameBuffer.h" "FrameBuffer.cxx"
	"reyes/GridShader.h" "reyes/GridShader.cxx"
	"reyes/MicroPolygon.h" "reyes/MicroPolygon.cxx"
	"reyes/MicroPolygonGrid.h" "reyes/MicroPolygonGrid.cxx"
	"Polygon.h" "Polygon.cxx"
	"Hider.h"
	"reyes/Hider.h" "reyes/Hider.cxx"
	"RayHider.h" "RayHider.cxx"
	"RenderContext.h" "RenderContext.cxx"
	"RIBHandler.h" "RIBHandler.cxx"
	"Renderer.h" "Renderer.cxx"
	"RenderMan.h" "RenderMan.cxx"
	"reyes/Primitive.h" "reyes/Primitive.cxx"
	"reyes/Samples.h" "reyes/Samples.cxx"
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
