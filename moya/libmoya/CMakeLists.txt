# Kernel code is a target of its own, compiled under the rules every device accepts.
add_subdirectory("kernel")

# What the CPU's libraries share with libmoya, and the libraries themselves, a target each.
add_subdirectory("device/cpu")

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

	"accel/Box.h"
	"accel/Assembly.h" "accel/Assembly.cxx"
	"accel/Builder.h" "accel/Builder.cxx"
	"accel/Hierarchy.h"
	"accel/MovingHierarchy.h"
	"accel/Options.h"
	"accel/Root.h"
	"accel/Sweep.h" "accel/Sweep.cxx"
	"accel/Triangles.h" "accel/Triangles.cxx"
	"accel/internal/Binary.h"
	"accel/internal/BinaryBuilder.h" "accel/internal/BinaryBuilder.cxx"
	"accel/internal/BinaryNode.h"
	"accel/internal/MovingBinary.h"
	"accel/internal/MovingBinaryNode.h"
	"accel/internal/MovingReference.h"
	"accel/internal/Reference.h"
	"accel/internal/TemporalBuilder.h" "accel/internal/TemporalBuilder.cxx"
	"accel/internal/Wave.h" "accel/internal/Wave.cxx"

	"camera/Camera.h" "camera/Camera.cxx"

	"colour/Transform.h" "colour/Transform.cxx"
	"colour/Config.h" "colour/Config.cxx"
	"colour/RgbTables.h"
	"colour/internal/Configuration.h"
	"colour/internal/Processor.h"
	"${V3D_MOYA_ACESCG_TABLE}"

	"device/Buffer.h"
	"device/CpuDevice.h" "device/CpuDevice.cxx"
	"device/CpuLibrary.h" "device/CpuLibrary.cxx"
	"device/Device.h"
	"device/internal/Arena.h"
	"device/internal/HostBuffer.h"

	"film/Channel.h"
	"film/Snapshot.h" "film/Snapshot.cxx"
	"film/Film.h" "film/Film.cxx"
	"film/Filter.h" "film/Filter.cxx"

	"geometry/Flat.h"
	"geometry/Geometry.h"
	"geometry/Group.h"
	"geometry/Instance.h"
	"geometry/Motion.h" "geometry/Motion.cxx"
	"geometry/Prototypes.h" "geometry/Prototypes.cxx"
	"geometry/Quadrics.h" "geometry/Quadrics.cxx"
	"geometry/Triangulate.h" "geometry/Triangulate.cxx"
	"geometry/World.h" "geometry/World.cxx"
	"geometry/internal/Walk.h" "geometry/internal/Walk.cxx"

	"output/ExrPart.h"
	"output/ExrMetadata.h"
	"output/Exr.h" "output/Exr.cxx"
	"output/Png.h" "output/Png.cxx"

	"ri/RenderMan.h" "ri/RenderMan.cxx"
	"ri/RIBHandler.h" "ri/RIBHandler.cxx"
	"ri/Renderer.h" "ri/Renderer.cxx"
	"ri/SceneClient.h" "ri/SceneClient.cxx"
	"ri/SceneWorld.cxx"
	"ri/internal/Converter.h" "ri/internal/Converter.cxx"
	"ri/internal/Primvars.h" "ri/internal/Primvars.cxx"

	"scene/Change.h"
	"scene/Declaration.h" "scene/Declaration.cxx"
	"scene/Description.h" "scene/Description.cxx"
	"scene/Handle.h" "scene/Handle.cxx"
	"scene/Kind.h"
	"scene/Kinds.h" "scene/Kinds.cxx"
	"scene/Placement.h" "scene/Placement.cxx"
	"scene/Samples.h"
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

	"view/Hits.h" "view/Hits.cxx"
	"view/Traced.h"
	"view/Tracing.h" "view/Tracing.cxx"

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
# types. The CPU device's header names the instruction sets its libraries are built for.
# PRIVATE: only Display writes an image, encoding it with image and writing the file through
# asset. Only the CPU device's source includes TBB, and only the EXR writer's source includes
# OpenEXR and Imath. Only the colour configuration's source and its internal headers include
# OpenColorIO.
target_link_libraries("v3dlib_moya" PUBLIC v3dlib_log v3dlib_moya_cpu v3dlib_moya_kernel v3dlib_render_offline v3dlib_type glm::glm)
target_link_libraries("v3dlib_moya" PRIVATE v3dlib_asset v3dlib_image Imath::Imath OpenColorIO::OpenColorIO OpenEXR::OpenEXR TBB::tbb)

# PRIVATE: only the CPU device's library loader opens a shared library, which needs libdl on some
# platforms, and finds it by its path.
target_link_libraries("v3dlib_moya" PRIVATE ${CMAKE_DL_LIBS} Boost::filesystem)
