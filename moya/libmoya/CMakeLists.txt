add_library("v3dlib_moya" 
	"Attributes.h" "Attributes.cxx"
	"reyes/Bucket.h" "reyes/Bucket.cxx"
	"reyes/Buckets.h" "reyes/Buckets.cxx"
	"reyes/Footprint.h" "reyes/Footprint.cxx"
	"Display.h" "Display.cxx"
	"reyes/GridShader.h" "reyes/GridShader.cxx"
	"reyes/MicroPolygon.h" "reyes/MicroPolygon.cxx"
	"reyes/MicroPolygonGrid.h" "reyes/MicroPolygonGrid.cxx"
	"Planes.h" "Planes.cxx"
	"Polygon.h" "Polygon.cxx"
	"Progress.h" "Progress.cxx"
	"Hider.h"
	"Lights.h" "Lights.cxx"
	"reyes/Hider.h" "reyes/Hider.cxx"
	"RayHider.h" "RayHider.cxx"
	"RenderContext.h" "RenderContext.cxx"
	"RIBHandler.h" "RIBHandler.cxx"
	"Renderer.h" "Renderer.cxx"
	"RenderMan.h" "RenderMan.cxx"
	"reyes/Primitive.h" "reyes/Primitive.cxx"
	"reyes/Samples.h" "reyes/Samples.cxx"
	"Shaders.h" "Shaders.cxx"
	"Shading.h"
	"TracedScene.h" "TracedScene.cxx"
	"Transforms.h" "Transforms.cxx"
	"Vertex.h" "Vertex.cxx"
)

# RenderMan.cxx defines the whole RenderMan Interface and leaves many of its entry points
# empty. Every parameter of an empty entry point is unreferenced, and at /W4 those C4100
# warnings would bury the rest of the tree's warnings. C4100 is disabled for this target, so
# a parameter keeps its name until its function gets a body. -Wunused-parameter is the same
# warning to Clang and GCC.
if(MSVC)
	target_compile_options("v3dlib_moya" PRIVATE /wd4100 /EHsc /utf-8)
else()
	target_compile_options("v3dlib_moya" PRIVATE -Wno-unused-parameter)
endif()

# PUBLIC: the framebuffer is part of the render context's interface, and the render context's
# header includes the log's. moya's headers name glm types. PRIVATE: only the render context's
# source writes images.
target_link_libraries("v3dlib_moya" PUBLIC v3dlib_log v3dlib_render_offline v3dlib_type glm::glm)
target_link_libraries("v3dlib_moya" PRIVATE v3dlib_image)
