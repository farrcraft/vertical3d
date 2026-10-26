v3d_add_api_library(render
	"realtime/Frame.h" "realtime/Frame.cpp"
	"realtime/SharedInstance.h" "realtime/SharedInstance.cpp"
	"realtime/WindowRenderer.h" "realtime/WindowRenderer.cpp" 
	"realtime/WindowContext.h" "realtime/WindowContext.cpp"
	"realtime/DeviceContext.h" "realtime/DeviceContext.cxx" 
	"realtime/Handle.h" "realtime/Registry.h" "realtime/UploadQueue.h"
	"realtime/canvas/Canvas.h" "realtime/canvas/Canvas.cpp"
	"realtime/canvas/Stacks.h" "realtime/canvas/Stacks.cpp"
	"realtime/canvas/Line.h" "realtime/canvas/Line.cpp"
	"realtime/canvas/World.h" "realtime/canvas/World.cpp"
	"realtime/canvas/DepthOrder.h" "realtime/canvas/DepthOrder.cpp"
	"realtime/entity/Billboard.h"
	"realtime/entity/Sprites.h" "realtime/entity/Sprites.cpp"
	"realtime/entity/Particles.h" "realtime/entity/Particles.cpp"
	"realtime/component/Sprite.h"
	"realtime/component/Particles.h"
	"realtime/component/Mesh.h"
	"realtime/DecodedModel.h" "realtime/DecodedModel.cpp"
	"realtime/MeshRegistry.h" "realtime/MeshRegistry.cpp"
	"realtime/Textures.h" "realtime/Textures.cpp"
	"realtime/entity/Meshes.h" "realtime/entity/Meshes.cpp"
	"realtime/entity/Poses.h" "realtime/entity/Poses.cpp"
	"realtime/lighting/Settings.h"
	"realtime/lighting/SceneUniforms.h" "realtime/lighting/SceneUniforms.cpp"
	"realtime/lighting/Shadow.h" "realtime/lighting/Shadow.cpp"
	"realtime/Grade.h" "realtime/Grade.cpp"
	"realtime/LitScene.h" "realtime/LitScene.cpp"
	"realtime/SortKey.h" "realtime/SortKey.cpp"
	"realtime/DrawItem.h" "realtime/DrawItem.cpp"
	"realtime/Pass.h" "realtime/Pass.cpp"
	"realtime/Present.h" "realtime/Present.cpp"
	"realtime/vulkan/device/Result.h" "realtime/vulkan/device/Result.cxx" 
	"realtime/vulkan/device/Instance.h" "realtime/vulkan/device/Instance.cxx" 
	"realtime/vulkan/device/Surface.h" "realtime/vulkan/device/Surface.cxx" 
	"realtime/vulkan/device/Device.h" "realtime/vulkan/device/Device.cxx" 
	"realtime/vulkan/frame/Swapchain.h" "realtime/vulkan/frame/Swapchain.cxx"
	"realtime/vulkan/frame/CommandPool.h" "realtime/vulkan/frame/CommandPool.cxx"
	"realtime/vulkan/pipeline/Cache.h" "realtime/vulkan/pipeline/Cache.cxx"
	"realtime/vulkan/frame/Presenter.h" "realtime/vulkan/frame/Presenter.cxx"
	"realtime/vulkan/frame/Ring.h" "realtime/vulkan/frame/Ring.cxx"
	"realtime/vulkan/frame/StreamRing.h" "realtime/vulkan/frame/StreamRing.cxx"
	"realtime/vulkan/frame/Timings.h" "realtime/vulkan/frame/Timings.cxx"
	"realtime/vulkan/frame/Retirement.h" "realtime/vulkan/frame/Retirement.cxx"
	"realtime/Recorder.h" "realtime/Recorder.cxx"
	"realtime/vulkan/pipeline/Pipeline.h" "realtime/vulkan/pipeline/Pipeline.cxx"
	"realtime/Material.h" "realtime/Material.cxx"
	"realtime/vulkan/pipeline/TextureArray.h" "realtime/vulkan/pipeline/TextureArray.cxx"
	"realtime/vulkan/pipeline/Texture.h"
	"realtime/Resources.h" "realtime/Resources.cxx"
	"realtime/vulkan/memory/Memory.h" "realtime/vulkan/memory/Memory.cxx"
	"realtime/vulkan/memory/Allocation.h" "realtime/vulkan/memory/Allocation.cxx"
	"realtime/vulkan/memory/Allocator.h" "realtime/vulkan/memory/Allocator.cxx"
	"realtime/vulkan/memory/VmaImpl.cxx"
	"realtime/vulkan/memory/Buffer.h" "realtime/vulkan/memory/Buffer.cxx"
	"realtime/vulkan/memory/Uploader.h" "realtime/vulkan/memory/Uploader.cxx"
	"realtime/vulkan/memory/DeviceBuffer.h" "realtime/vulkan/memory/DeviceBuffer.cxx"
	"realtime/Mesh.h" "realtime/Mesh.cxx"
	"realtime/vulkan/memory/Image.h" "realtime/vulkan/memory/Image.cxx"
	"realtime/vulkan/pipeline/Sampler.h" "realtime/vulkan/pipeline/Sampler.cxx"
	"realtime/vulkan/pipeline/DescriptorPool.h" "realtime/vulkan/pipeline/DescriptorPool.cxx"
	"realtime/vulkan/frame/DepthBuffer.h" "realtime/vulkan/frame/DepthBuffer.cxx"
	"realtime/RenderTarget.h" "realtime/RenderTarget.cxx"
	"realtime/vulkan/frame/FrameUniforms.h" "realtime/vulkan/frame/FrameUniforms.cxx"
	"realtime/vulkan/frame/Capture.h" "realtime/vulkan/frame/Capture.cxx"
	"realtime/vulkan/pipeline/Formats.h"
	"realtime/vulkan/pipeline/Builder.h" "realtime/vulkan/pipeline/Builder.cxx"
	"realtime/vulkan/memory/Barriers.h" "realtime/vulkan/memory/Barriers.cxx"
	"realtime/vulkan/memory/TextureFactory.h" "realtime/vulkan/memory/TextureFactory.cxx"
	"realtime/renderer/Clip.h"
	"realtime/renderer/Quad.h" "realtime/renderer/Quad.cxx"
	"realtime/renderer/Line.h" "realtime/renderer/Line.cxx"
	"realtime/renderer/World.h" "realtime/renderer/World.cxx"
	"realtime/renderer/Lit.h" "realtime/renderer/Lit.cxx"
	"realtime/renderer/FullScreen.h" "realtime/renderer/FullScreen.cxx")

# The batched quad primitive. Every shader is compiled to SPIR-V and embedded, not loaded
# from disk, by v3d_add_shader in cmake/v3dHelpers.cmake.
v3d_add_shader(v3dlib_render "shaders/quad.vert")
v3d_add_shader(v3dlib_render "shaders/quad.frag")
# the same stage for an _SRGB target, which decodes before writing so the target's encode on store
# gives back the colour the quad was given
v3d_add_shader(v3dlib_render "shaders/quad.frag" OUTPUT "linear_quad.frag" DEFINES LINEARISE)

# The world space line primitive.
v3d_add_shader(v3dlib_render "shaders/line.vert")
v3d_add_shader(v3dlib_render "shaders/line.frag")

# The world space quad primitive.
v3d_add_shader(v3dlib_render "shaders/world.vert")
v3d_add_shader(v3dlib_render "shaders/world.frag")

# The lit renderer. The blocks its shaders share are in shaders/lit/lit.glsl.
v3d_add_shader(v3dlib_render "shaders/lit/mesh.vert")
v3d_add_shader(v3dlib_render "shaders/lit/cel.frag")
v3d_add_shader(v3dlib_render "shaders/lit/outline.vert")
v3d_add_shader(v3dlib_render "shaders/lit/outline.frag")
v3d_add_shader(v3dlib_render "shaders/lit/shadow.vert")
# A skinned model uses the same vertex shaders compiled with SKINNED, which
# shaders/lit/pose.glsl reads
v3d_add_shader(v3dlib_render "shaders/lit/mesh.vert" OUTPUT "skinned_mesh.vert" DEFINES SKINNED)
v3d_add_shader(v3dlib_render "shaders/lit/outline.vert" OUTPUT "skinned_outline.vert" DEFINES SKINNED)
v3d_add_shader(v3dlib_render "shaders/lit/shadow.vert" OUTPUT "skinned_shadow.vert" DEFINES SKINNED)

# The pass over a whole target, and the colour grade drawn with it.
v3d_add_shader(v3dlib_render "shaders/fullscreen.vert")
v3d_add_shader(v3dlib_render "shaders/grade.frag")
# The grade compiled for each pairing of what it reads and what it draws into. LINEAR_SCENE reads
# a scene from an _SRGB target, which arrives decoded, and LINEAR_TARGET draws into one, which
# encodes on store. shaders/grade.frag reads both
v3d_add_shader(v3dlib_render "shaders/grade.frag" OUTPUT "grade_linear_scene.frag" DEFINES LINEAR_SCENE)
v3d_add_shader(v3dlib_render "shaders/grade.frag" OUTPUT "grade_linear_target.frag" DEFINES LINEAR_TARGET)
v3d_add_shader(v3dlib_render "shaders/grade.frag" OUTPUT "grade_linear.frag" DEFINES LINEAR_SCENE LINEAR_TARGET)

# PUBLIC: these sources compile Logger.h, and so do their consumers.
# v3dlib_image: Grade.h takes the image::Image a colour grade is built from.
# v3dlib_window: WindowRenderer.h and WindowContext.h take the window they present to.
# Vulkan and SDL3 are PUBLIC because the realtime headers name their types outright:
# WindowContext.h and every vulkan/ header carry VkDevice and its neighbours, and Surface.h
# carries SDL_Window.
target_link_libraries(v3dlib_render PUBLIC v3dlib_log v3dlib_asset v3dlib_image v3dlib_type v3dlib_window Vulkan::Vulkan SDL3::SDL3 glm::glm EnTT::EnTT)
# PRIVATE: the allocator is an implementation detail of memory::Allocator, and Allocator.h and
# Allocation.h declare the handles they name rather than including the header that defines them
target_link_libraries(v3dlib_render PRIVATE GPUOpen::VulkanMemoryAllocator)

# VMA is one header that is both declaration and implementation, compiled in exactly one
# translation unit. Its warnings and both analysers are switched off, as api/asset/media does
# for cgltf, because a local fix would be lost the next time the port is updated.
set_source_files_properties("realtime/vulkan/memory/VmaImpl.cxx" PROPERTIES SKIP_LINTING TRUE)
if(MSVC)
	set_source_files_properties("realtime/vulkan/memory/VmaImpl.cxx" PROPERTIES COMPILE_OPTIONS "/analyze-;/W0")
else()
	# -w is /W0. Clang and GCC have no analyser of /analyze's kind to switch off
	set_source_files_properties("realtime/vulkan/memory/VmaImpl.cxx" PROPERTIES COMPILE_OPTIONS "-w")
endif()

# PRIVATE: Canvas.h forward declares font::TextBuffer rather than including it, so a consumer
# needs the font library only if it names one itself. The ecs components the lit renderer
# reads are named in its sources and in no header, and so are the model and image assets
# DecodedModel loads. DecodedModel.cpp names a texture beside its model with
# boost::filesystem.
target_link_libraries(v3dlib_render PRIVATE v3dlib_font v3dlib_ecs v3dlib_asset_media Boost::filesystem)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
