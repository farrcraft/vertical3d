add_library("v3dlib_moya"
	"Curve.h" "Curve.cxx"
	"Hider.h"
	"Polygon.h" "Polygon.cxx"
	"Vertex.h" "Vertex.cxx"

	"ri/RenderMan.h" "ri/RenderMan.cxx"
	"ri/RIBHandler.h" "ri/RIBHandler.cxx"
	"ri/Renderer.h" "ri/Renderer.cxx"
	"ri/internal/Converter.h" "ri/internal/Converter.cxx"

	"state/Attributes.h" "state/Attributes.cxx"
	"state/Display.h" "state/Display.cxx"
	"state/Lights.h" "state/Lights.cxx"
	"state/Planes.h" "state/Planes.cxx"
	"state/Progress.h" "state/Progress.cxx"
	"state/RenderContext.h" "state/RenderContext.cxx"
	"state/Shaders.h" "state/Shaders.cxx"
	"state/Shading.h"
	"state/Transforms.h" "state/Transforms.cxx"

	"raytrace/Hider.h" "raytrace/Hider.cxx"
	"raytrace/Pass.h"
	"raytrace/Pixel.h"
	"raytrace/Spread.h"
	"raytrace/Scene.h" "raytrace/Scene.cxx"

	"reyes/Bucket.h" "reyes/Bucket.cxx"
	"reyes/Buckets.h" "reyes/Buckets.cxx"
	"reyes/Footprint.h" "reyes/Footprint.cxx"
	"reyes/GridShader.h" "reyes/GridShader.cxx"
	"reyes/Hider.h" "reyes/Hider.cxx"
	"reyes/MicroPolygon.h" "reyes/MicroPolygon.cxx"
	"reyes/MicroPolygonGrid.h" "reyes/MicroPolygonGrid.cxx"
	"reyes/Motions.h" "reyes/Motions.cxx"
	"reyes/Placement.h" "reyes/Placement.cxx"
	"reyes/Primitive.h" "reyes/Primitive.cxx"
	"reyes/Samples.h" "reyes/Samples.cxx"
)

# ri/RenderMan.cxx defines the whole RenderMan Interface and leaves many of its entry points
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
# header includes the log's. moya's headers name glm types. PRIVATE: only Display writes an image,
# encoding it with image and writing the file through asset.
target_link_libraries("v3dlib_moya" PUBLIC v3dlib_log v3dlib_render_offline v3dlib_type glm::glm)
target_link_libraries("v3dlib_moya" PRIVATE v3dlib_asset v3dlib_image)
