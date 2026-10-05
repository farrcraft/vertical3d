v3d_add_api_library(render
	"realtime/Frame.h" "realtime/Frame.cpp"
	"realtime/Engine.h" "realtime/Engine.cpp" 
	"realtime/Window.h" "realtime/Window.cpp" 
	"realtime/Engine3D.h" "realtime/Engine3D.cpp" 
	"realtime/Context3D.h" "realtime/Context3D.cpp"
	"realtime/DeviceContext.h" "realtime/DeviceContext.cxx" 
	"realtime/Handle.h" "realtime/Registry.h"
	"realtime/Canvas.h" "realtime/Canvas.cpp"
	"realtime/LineCanvas.h" "realtime/LineCanvas.cpp"
	"realtime/WorldCanvas.h" "realtime/WorldCanvas.cpp"
	"realtime/DepthOrder.h" "realtime/DepthOrder.cpp"
	"realtime/Sprites.h" "realtime/Sprites.cpp"
	"realtime/Particles.h" "realtime/Particles.cpp"
	"realtime/component/Sprite.h"
	"realtime/component/Particles.h"
	"realtime/component/Mesh.h"
	"realtime/MeshRegistry.h" "realtime/MeshRegistry.cpp"
	"realtime/Meshes.h" "realtime/Meshes.cpp"
	"realtime/Poses.h" "realtime/Poses.cpp"
	"realtime/LitSettings.h"
	"realtime/SceneUniforms.h" "realtime/SceneUniforms.cpp"
	"realtime/Shadow.h" "realtime/Shadow.cpp"
	"realtime/Grade.h" "realtime/Grade.cpp"
	"realtime/SortKey.h" "realtime/SortKey.cpp"
	"realtime/DrawItem.h" "realtime/DrawItem.cpp"
	"realtime/Pass.h" "realtime/Pass.cpp"
	"realtime/vulkan/device/Result.h" "realtime/vulkan/device/Result.cxx" 
	"realtime/vulkan/device/Instance.h" "realtime/vulkan/device/Instance.cxx" 
	"realtime/vulkan/device/Surface.h" "realtime/vulkan/device/Surface.cxx" 
	"realtime/vulkan/device/Device.h" "realtime/vulkan/device/Device.cxx" 
	"realtime/vulkan/frame/Swapchain.h" "realtime/vulkan/frame/Swapchain.cxx"
	"realtime/vulkan/frame/CommandPool.h" "realtime/vulkan/frame/CommandPool.cxx"
	"realtime/vulkan/pipeline/Cache.h" "realtime/vulkan/pipeline/Cache.cxx"
	"realtime/vulkan/frame/Presenter.h" "realtime/vulkan/frame/Presenter.cxx"
	"realtime/vulkan/frame/Ring.h" "realtime/vulkan/frame/Ring.cxx"
	"realtime/vulkan/frame/Timings.h" "realtime/vulkan/frame/Timings.cxx"
	"realtime/vulkan/frame/Retirement.h" "realtime/vulkan/frame/Retirement.cxx"
	"realtime/vulkan/frame/Recorder.h" "realtime/vulkan/frame/Recorder.cxx"
	"realtime/vulkan/pipeline/Pipeline.h" "realtime/vulkan/pipeline/Pipeline.cxx"
	"realtime/vulkan/pipeline/Material.h" "realtime/vulkan/pipeline/Material.cxx"
	"realtime/vulkan/pipeline/Texture.h"
	"realtime/vulkan/pipeline/Resources.h" "realtime/vulkan/pipeline/Resources.cxx"
	"realtime/vulkan/memory/Memory.h" "realtime/vulkan/memory/Memory.cxx"
	"realtime/vulkan/memory/Allocation.h" "realtime/vulkan/memory/Allocation.cxx"
	"realtime/vulkan/memory/Allocator.h" "realtime/vulkan/memory/Allocator.cxx"
	"realtime/vulkan/memory/VmaImpl.cxx"
	"realtime/vulkan/memory/Buffer.h" "realtime/vulkan/memory/Buffer.cxx"
	"realtime/vulkan/memory/Uploader.h" "realtime/vulkan/memory/Uploader.cxx"
	"realtime/vulkan/memory/DeviceBuffer.h" "realtime/vulkan/memory/DeviceBuffer.cxx"
	"realtime/vulkan/memory/Mesh.h" "realtime/vulkan/memory/Mesh.cxx"
	"realtime/vulkan/memory/Image.h" "realtime/vulkan/memory/Image.cxx"
	"realtime/vulkan/pipeline/Sampler.h" "realtime/vulkan/pipeline/Sampler.cxx"
	"realtime/vulkan/pipeline/DescriptorPool.h" "realtime/vulkan/pipeline/DescriptorPool.cxx"
	"realtime/vulkan/frame/DepthBuffer.h" "realtime/vulkan/frame/DepthBuffer.cxx"
	"realtime/vulkan/frame/RenderTarget.h" "realtime/vulkan/frame/RenderTarget.cxx"
	"realtime/vulkan/frame/FrameUniforms.h" "realtime/vulkan/frame/FrameUniforms.cxx"
	"realtime/vulkan/frame/Capture.h" "realtime/vulkan/frame/Capture.cxx"
	"realtime/vulkan/pipeline/Builder.h" "realtime/vulkan/pipeline/Builder.cxx"
	"realtime/vulkan/memory/TextureFactory.h" "realtime/vulkan/memory/TextureFactory.cxx"
	"realtime/vulkan/renderer/Quad.h" "realtime/vulkan/renderer/Quad.cxx"
	"realtime/vulkan/renderer/Line.h" "realtime/vulkan/renderer/Line.cxx"
	"realtime/vulkan/renderer/World.h" "realtime/vulkan/renderer/World.cxx"
	"realtime/vulkan/renderer/Lit.h" "realtime/vulkan/renderer/Lit.cxx"
	"realtime/vulkan/renderer/FullScreen.h" "realtime/vulkan/renderer/FullScreen.cxx")

# The one batched quad primitive of ADR-0005. Compiled to SPIR-V and embedded rather than
# loaded from disk - see v3d_add_shader in the root CMakeLists.
v3d_add_shader(v3dlib_render "shaders/quad.vert")
v3d_add_shader(v3dlib_render "shaders/quad.frag")

# The line primitive of ADR-0011.
v3d_add_shader(v3dlib_render "shaders/line.vert")
v3d_add_shader(v3dlib_render "shaders/line.frag")

# The world space quad primitive of ADR-0042.
v3d_add_shader(v3dlib_render "shaders/world.vert")
v3d_add_shader(v3dlib_render "shaders/world.frag")

# The lit tier of ADR-0064, whose shared blocks are shaders/lit/lit.glsl - ADR-0067.
v3d_add_shader(v3dlib_render "shaders/lit/mesh.vert")
v3d_add_shader(v3dlib_render "shaders/lit/cel.frag")
v3d_add_shader(v3dlib_render "shaders/lit/outline.vert")
v3d_add_shader(v3dlib_render "shaders/lit/outline.frag")
v3d_add_shader(v3dlib_render "shaders/lit/shadow.vert")
v3d_add_shader(v3dlib_render "shaders/lit/skinned_mesh.vert")
v3d_add_shader(v3dlib_render "shaders/lit/skinned_outline.vert")
v3d_add_shader(v3dlib_render "shaders/lit/skinned_shadow.vert")

# The pass over a whole target, and the colour grade drawn with it.
v3d_add_shader(v3dlib_render "shaders/fullscreen.vert")
v3d_add_shader(v3dlib_render "shaders/grade.frag")

# PUBLIC: these sources compile Logger.h, and so do their consumers.
# v3dlib_image: Grade.h takes the image::Image a colour grade is built from.
# Vulkan and SDL3 are PUBLIC because the realtime headers name their types outright -
# Context3D.h and every vulkan/ header carry VkDevice and its neighbours, and Window.h
# carries SDL_Window.
target_link_libraries(v3dlib_render PUBLIC v3dlib_log v3dlib_asset v3dlib_image v3dlib_type Vulkan::Vulkan SDL3::SDL3 glm::glm EnTT::EnTT)
# PRIVATE: the allocator is an implementation detail of memory::Allocator, and Allocator.h and
# Allocation.h declare the handles they name rather than including the header that defines them
target_link_libraries(v3dlib_render PRIVATE GPUOpen::VulkanMemoryAllocator)

# one header that is both declaration and implementation, compiled in exactly one translation
# unit. Dropping the warning level and both analysers is what api/asset does for cgltf, and for
# the same reason: a local fix is lost the next time the port moves.
set_source_files_properties("realtime/vulkan/memory/VmaImpl.cxx" PROPERTIES
	COMPILE_OPTIONS "/analyze-;/W0"
	SKIP_LINTING TRUE)

# PRIVATE: Canvas.h forward declares font::TextBuffer rather than including it, so a consumer
# needs the font library only if it names one itself. The ecs components the lit renderer
# walks are named in its sources and in no header.
target_link_libraries(v3dlib_render PRIVATE v3dlib_font v3dlib_ecs)

target_compile_options(v3dlib_render PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
