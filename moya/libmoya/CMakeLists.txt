# Kernel code is a target of its own, compiled under the rules every device accepts.
add_subdirectory("kernel")

# The RGB-to-spectrum table, which moya_rgbtable fits when the tool or the kernel's colour code
# changes. The output is generated, so it is not linted: clang-tidy and /analyze would spend
# minutes on its millions of literals, which hold no code.
set(V3D_MOYA_ACESCG_TABLE "${CMAKE_CURRENT_BINARY_DIR}/colour/AcescgTable.cxx")
add_custom_command(OUTPUT "${V3D_MOYA_ACESCG_TABLE}"
	COMMAND moya_rgbtable "${V3D_MOYA_ACESCG_TABLE}"
	DEPENDS moya_rgbtable
	COMMENT "Fitting the ACEScg RGB-to-spectrum table"
	VERBATIM)
set_source_files_properties("${V3D_MOYA_ACESCG_TABLE}" PROPERTIES SKIP_LINTING ON)
if(MSVC)
	set_source_files_properties("${V3D_MOYA_ACESCG_TABLE}" PROPERTIES COMPILE_OPTIONS "/analyze-")
endif()

add_library("v3dlib_moya"
	"Curve.h" "Curve.cxx"
	"Hider.h"
	"Polygon.h" "Polygon.cxx"
	"Vertex.h" "Vertex.cxx"

	"colour/Config.h" "colour/Config.cxx"
	"colour/RgbTables.h"
	"colour/internal/Configuration.h"
	"colour/internal/Processor.h"
	"${V3D_MOYA_ACESCG_TABLE}"

	"device/Buffer.h"
	"device/CpuDevice.h" "device/CpuDevice.cxx"
	"device/Device.h"
	"device/internal/Arena.h"
	"device/internal/HostBuffer.h"

	"film/Film.h" "film/Film.cxx"
	"film/Filter.h" "film/Filter.cxx"

	"output/Exr.h" "output/Exr.cxx"
	"output/Png.h" "output/Png.cxx"

	"ri/RenderMan.h" "ri/RenderMan.cxx"
	"ri/RIBHandler.h" "ri/RIBHandler.cxx"
	"ri/Renderer.h" "ri/Renderer.cxx"
	"ri/SceneClient.h" "ri/SceneClient.cxx"
	"ri/SceneWorld.cxx"
	"ri/internal/Converter.h" "ri/internal/Converter.cxx"
	"ri/internal/Primvars.h" "ri/internal/Primvars.cxx"

	"scene/Description.h" "scene/Description.cxx"
	"scene/Kinds.h" "scene/Kinds.cxx"
	"scene/Placement.h" "scene/Placement.cxx"
	"scene/Scene.h" "scene/Scene.cxx"
	"scene/Schema.h" "scene/Schema.cxx"
	"scene/Value.h" "scene/Value.cxx"

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
# header includes the log's. moya's headers name glm types, and the device's headers name kernel
# types. PRIVATE: only Display writes an image, encoding it with image and writing the file through
# asset. Only the CPU device's source includes TBB, and only the EXR writer's source includes
# OpenEXR and Imath. Only the colour configuration's source and its internal headers include
# OpenColorIO.
target_link_libraries("v3dlib_moya" PUBLIC v3dlib_log v3dlib_moya_kernel v3dlib_render_offline v3dlib_type glm::glm)
target_link_libraries("v3dlib_moya" PRIVATE v3dlib_asset v3dlib_image Imath::Imath OpenColorIO::OpenColorIO OpenEXR::OpenEXR TBB::tbb)
