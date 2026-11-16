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

# The GGX albedo tables energy compensation reads, which moya_albedo integrates when the tool, the
# kernel's GGX or its Fresnel changes. The output is generated, so it is not linted.
set(V3D_MOYA_GGX_ALBEDOS "${CMAKE_CURRENT_BINARY_DIR}/material/GgxAlbedos.cxx")
add_custom_command(OUTPUT "${V3D_MOYA_GGX_ALBEDOS}"
	COMMAND moya_albedo "${V3D_MOYA_GGX_ALBEDOS}"
	DEPENDS moya_albedo
	COMMENT "Integrating the GGX albedo tables"
	VERBATIM)
set_source_files_properties("${V3D_MOYA_GGX_ALBEDOS}" PROPERTIES SKIP_LINTING ON)
if(MSVC)
	set_source_files_properties("${V3D_MOYA_GGX_ALBEDOS}" PROPERTIES COMPILE_OPTIONS "/analyze-")
endif()

# OpenSubdiv's scheme headers use M_PI, which MSVC's <cmath> defines only when it is asked to.
if(MSVC)
	set_source_files_properties("geometry/subdivision/Surface.cxx" PROPERTIES COMPILE_DEFINITIONS "_USE_MATH_DEFINES")
endif()

add_library("v3dlib_moya"
	"Curve.h" "Curve.cxx"
	"Executable.h" "Executable.cxx"
	"Hider.h"
	"Polygon.h" "Polygon.cxx"
	"Vertex.h" "Vertex.cxx"

	"accel/Assembler.h" "accel/Assembler.cxx"
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
	"accel/internal/Part.h"
	"accel/internal/Parts.h" "accel/internal/Parts.cxx"
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

	"denoise/Denoise.h" "denoise/Denoise.cxx"
	"device/Buffer.h"
	"device/CpuDevice.h" "device/CpuDevice.cxx"
	"device/CpuLibrary.h" "device/CpuLibrary.cxx"
	"device/Device.h"
	"device/internal/Arena.h"
	"device/internal/HostBuffer.h"

	"film/Channel.h"
	"film/Compare.h" "film/Compare.cxx"
	"film/Covering.h"
	"film/DeepPoint.h"
	"film/DeepSample.h"
	"film/Estimate.h"
	"film/Snapshot.h" "film/Snapshot.cxx"
	"film/State.h"
	"film/Film.h" "film/Film.cxx"
	"film/Filter.h" "film/Filter.cxx"

	"geometry/Cache.h"
	"geometry/ConvertedMaterials.h" "geometry/ConvertedMaterials.cxx"
	"geometry/Curves.h" "geometry/Curves.cxx"
	"geometry/Flat.h"
	"geometry/Geometry.h"
	"geometry/Group.h"
	"geometry/Instance.h"
	"geometry/Motion.h" "geometry/Motion.cxx"
	"geometry/Prototypes.h" "geometry/Prototypes.cxx"
	"geometry/Quadrics.h" "geometry/Quadrics.cxx"
	"geometry/Segments.h"
	"geometry/ShapedLight.h" "geometry/ShapedLight.cxx"
	"geometry/Triangulate.h" "geometry/Triangulate.cxx"
	"geometry/Surfaces.h" "geometry/Surfaces.cxx"
	"integrate/Profile.h"
	"integrate/Render.h" "integrate/Render.cxx"
	"integrate/Trails.h"
	"integrate/internal/Coverage.h" "integrate/internal/Coverage.cxx"
	"integrate/internal/Crop.h" "integrate/internal/Crop.cxx"
	"integrate/internal/Deep.h" "integrate/internal/Deep.cxx"
	"integrate/internal/Kept.h"
	"integrate/internal/Names.h" "integrate/internal/Names.cxx"
	"integrate/internal/Records.h" "integrate/internal/Records.cxx"
	"integrate/internal/FirstHits.h" "integrate/internal/FirstHits.cxx"
	"integrate/internal/LightPaths.h" "integrate/internal/LightPaths.cxx"
	"integrate/internal/Tallies.h" "integrate/internal/Tallies.cxx"
	"integrate/internal/TrailSteps.h" "integrate/internal/TrailSteps.cxx"
	"light/Ies.h" "light/Ies.cxx"
	"light/Lights.h" "light/Lights.cxx"
	"light/internal/Gathered.h"
	"light/internal/Profiles.h" "light/internal/Profiles.cxx"
	"light/internal/Linking.h" "light/internal/Linking.cxx"
	"light/internal/LinkSets.h" "light/internal/LinkSets.cxx"
	"light/internal/Pending.h"
	"light/internal/Subtree.h"
	"light/internal/Textures.h" "light/internal/Textures.cxx"
	"light/internal/Tree.h" "light/internal/Tree.cxx"
	"light/Planckian.h" "light/Planckian.cxx"
	"lpe/Automaton.h" "lpe/Automaton.cxx"
	"lpe/Compile.h" "lpe/Compile.cxx"
	"lpe/Expression.h" "lpe/Expression.cxx"
	"lpe/Failure.h"
	"lpe/Pattern.h"
	"lpe/Refused.h"
	"lpe/internal/Reader.h"
	"lpe/internal/Threads.h"
	"geometry/World.h" "geometry/World.cxx"
	"geometry/internal/Displace.h" "geometry/internal/Displace.cxx"
	"geometry/internal/LightShapes.h" "geometry/internal/LightShapes.cxx"
	"geometry/internal/Piece.h"
	"geometry/internal/Primvars.h" "geometry/internal/Primvars.cxx"
	"geometry/internal/Programs.h" "geometry/internal/Programs.cxx"
	"geometry/internal/Walk.h" "geometry/internal/Walk.cxx"
	"geometry/subdivision/Dicing.h" "geometry/subdivision/Dicing.cxx"
	"geometry/subdivision/Surface.h" "geometry/subdivision/Surface.cxx"
	"geometry/subdivision/Tessellation.h"
	"geometry/subdivision/Topology.h"
	"geometry/subdivision/internal/Cutter.h" "geometry/subdivision/internal/Cutter.cxx"
	"geometry/subdivision/internal/Patches.h"
	"geometry/subdivision/internal/Scalar.h"

	"material/Albedos.h"
	"material/Compile.h" "material/Compile.cxx"
	"material/Documents.h" "material/Documents.cxx"
	"material/Fold.h" "material/Fold.cxx"
	"${V3D_MOYA_GGX_ALBEDOS}"
	"material/Library.h" "material/Library.cxx"
	"material/internal/Definitions.h"
	"material/internal/Emitter.h" "material/internal/Emitter.cxx"
	"material/internal/Translate.h" "material/internal/Translate.cxx"
	"material/internal/Translation.h"
	"material/internal/TranslationGraphs.h" "material/internal/TranslationGraphs.cxx"
	"material/internal/RiSurfaces.h" "material/internal/RiSurfaces.cxx"
	"material/internal/RiParameter.h"
	"material/internal/RiFeed.h"
	"material/internal/Network.h"
	"material/internal/TerminalInput.h"
	"material/internal/Operation.h" "material/internal/Operation.cxx"

	"output/Checkpoint.h" "output/Checkpoint.cxx"
	"output/Cryptomatte.h" "output/Cryptomatte.cxx"
	"output/DeepExr.h" "output/DeepExr.cxx"
	"output/internal/Entries.h"
	"output/internal/Field.h"
	"output/internal/ExrWindows.h"
	"output/ExrPart.h"
	"output/ExrMetadata.h"
	"output/Chromaticities.h"
	"output/Exr.h" "output/Exr.cxx"
	"output/ExrImage.h" "output/ExrImage.cxx"
	"output/Png.h" "output/Png.cxx"
	"output/Channel.h"
	"output/Output.h"
	"output/Outputs.h" "output/Outputs.cxx"

	"ri/RIBHandler.h" "ri/RIBHandler.cxx"
	"ri/SceneClient.h" "ri/SceneClient.cxx"
	"ri/internal/Deforming.h" "ri/internal/Deforming.cxx"
	"ri/internal/Document.h" "ri/internal/Document.cxx"
	"ri/internal/DocumentReader.h" "ri/internal/DocumentReader.cxx"
	"ri/internal/Lights.h" "ri/internal/Lights.cxx"
	"ri/internal/Objects.h" "ri/internal/Objects.cxx"
	"ri/internal/Primvars.h" "ri/internal/Primvars.cxx"
	"ri/internal/Sampled.h" "ri/internal/Sampled.cxx"
	"ri/internal/Shape.h"
	"ri/internal/Shapes.h" "ri/internal/Shapes.cxx"
	"ri/internal/Warnings.h" "ri/internal/Warnings.cxx"

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

	"texture/Cache.h" "texture/Cache.cxx"
	"texture/Level.h"
	"texture/Maker.h" "texture/Maker.cxx"
	"texture/Options.h" "texture/Options.cxx"
	"texture/Request.h"
	"texture/SearchPath.h" "texture/SearchPath.cxx"
	"texture/Textures.h" "texture/Textures.cxx"
	"texture/internal/Opened.h"
	"texture/internal/Picture.h"
	"texture/internal/Taps.h"
	"texture/internal/Tile.h"
	"texture/internal/TileKey.h"

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

# MSVC needs /EHsc for C++ exceptions, and /utf-8 for the fmt that spdlog bundles. Clang and
# GCC need neither.
if(MSVC)
	target_compile_options("v3dlib_moya" PRIVATE /EHsc /utf-8)
endif()

# PUBLIC: the framebuffer is part of the render context's interface, and the render context's
# header includes the log's. moya's headers name glm types, and the device's headers name kernel
# types. The CPU device's header names the instruction sets its libraries are built for.
# PRIVATE: Display and the PNG writer encode an image with image and write the file through
# asset, and the texture maker reads one with image. Only the CPU device's and the texture
# maker's sources include TBB, and only the EXR files' sources and the texture maker's include
# OpenEXR and Imath. Only the colour configuration's source and its internal headers include
# OpenColorIO, only the subdivision surface's OpenSubdiv, and only the material library's source
# and its internal header MaterialX, and only the denoiser's source OIDN. OpenSubdiv is linked
# static on every platform, as vcpkg builds it on Windows.
target_link_libraries("v3dlib_moya" PUBLIC v3dlib_log v3dlib_moya_cpu v3dlib_moya_kernel v3dlib_render_offline v3dlib_type glm::glm)
target_link_libraries("v3dlib_moya" PRIVATE v3dlib_asset v3dlib_image Imath::Imath OpenColorIO::OpenColorIO OpenEXR::OpenEXR OpenSubdiv::osdCPU_static MaterialXCore MaterialXFormat OpenImageDenoise TBB::tbb)

# PRIVATE: only the CPU device's library loader opens a shared library, which needs libdl on some
# platforms, and finds it by its path.
target_link_libraries("v3dlib_moya" PRIVATE ${CMAKE_DL_LIBS} Boost::filesystem)

# MaterialX's standard libraries: its node definitions and the graphs that implement some of them,
# without the shader generators' implementations, which moya does not use. They are staged in the
# build tree when it is configured, and installed from there. The staged copy is made afresh, so a
# file a newer MaterialX drops is not left behind.
file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/materialx")
file(COPY "${MATERIALX_STDLIB_DIR}/" DESTINATION "${CMAKE_BINARY_DIR}/materialx"
	PATTERN "gen*" EXCLUDE
	PATTERN "targets" EXCLUDE)

# Copies MaterialX's standard libraries into materialx/ beside an executable that reads materials,
# where material::Library finds them. The copy is a target of its own that runs on every build, so
# libraries staged again are copied even when the executable is not linked again. It replaces the
# directory, so a file no longer staged is not left behind.
#
# @param target the executable
function(v3d_moya_materialx_libraries target)
	add_custom_target("${target}_materialx_libraries"
		COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:${target}>/materialx"
		COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/materialx" "$<TARGET_FILE_DIR:${target}>/materialx"
		VERBATIM)
	add_dependencies(${target} "${target}_materialx_libraries")
endfunction()
