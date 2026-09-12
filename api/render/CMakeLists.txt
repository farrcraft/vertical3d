v3d_add_api_library(render
	"realtime/Frame.h" "realtime/Frame.cpp"
	"realtime/Engine.h" "realtime/Engine.cpp" 
	"realtime/Context.h" "realtime/Window.h" "realtime/Window.cpp" 
	"realtime/Engine3D.h" "realtime/Engine3D.cpp" 
	"realtime/Context3D.h" "realtime/Context3D.cpp"
	"realtime/DeviceContext.h" "realtime/DeviceContext.cxx" 
	"realtime/Handle.h" "realtime/Registry.h"
	"realtime/Canvas.h" "realtime/Canvas.cpp"
	"realtime/LineCanvas.h" "realtime/LineCanvas.cpp"
	"realtime/WorldCanvas.h" "realtime/WorldCanvas.cpp"
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
	"realtime/vulkan/frame/Recorder.h" "realtime/vulkan/frame/Recorder.cxx"
	"realtime/vulkan/pipeline/Resources.h" "realtime/vulkan/pipeline/Resources.cxx"
	"realtime/vulkan/memory/Memory.h" "realtime/vulkan/memory/Memory.cxx"
	"realtime/vulkan/memory/Allocator.h" "realtime/vulkan/memory/Allocator.cxx"
	"realtime/vulkan/memory/Buffer.h" "realtime/vulkan/memory/Buffer.cxx"
	"realtime/vulkan/memory/Uploader.h" "realtime/vulkan/memory/Uploader.cxx"
	"realtime/vulkan/memory/DeviceBuffer.h" "realtime/vulkan/memory/DeviceBuffer.cxx"
	"realtime/vulkan/memory/Mesh.h" "realtime/vulkan/memory/Mesh.cxx"
	"realtime/vulkan/frame/DepthBuffer.h" "realtime/vulkan/frame/DepthBuffer.cxx"
	"realtime/vulkan/frame/RenderTarget.h" "realtime/vulkan/frame/RenderTarget.cxx"
	"realtime/vulkan/frame/FrameUniforms.h" "realtime/vulkan/frame/FrameUniforms.cxx"
	"realtime/vulkan/frame/Capture.h" "realtime/vulkan/frame/Capture.cxx"
	"realtime/vulkan/pipeline/Builder.h" "realtime/vulkan/pipeline/Builder.cxx"
	"realtime/vulkan/memory/TextureFactory.h" "realtime/vulkan/memory/TextureFactory.cxx"
	"realtime/vulkan/renderer/Quad.h" "realtime/vulkan/renderer/Quad.cxx"
	"realtime/vulkan/renderer/Line.h" "realtime/vulkan/renderer/Line.cxx"
	"realtime/vulkan/renderer/World.h" "realtime/vulkan/renderer/World.cxx")

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

# PUBLIC: these sources compile Logger.h, and so do their consumers.
# Vulkan and SDL3 are PUBLIC because the realtime headers name their types outright -
# Context3D.h and every vulkan/ header carry VkDevice and its neighbours, and Window.h
# carries SDL_Window.
target_link_libraries(v3dlib_render PUBLIC v3dlib_log v3dlib_asset Vulkan::Vulkan SDL3::SDL3 glm::glm EnTT::EnTT)

# PRIVATE: Canvas.h and TextureFactory.h forward declare font::TextBuffer and image::Image
# rather than including them, so a consumer needs these only if it names one itself.
target_link_libraries(v3dlib_render PRIVATE v3dlib_font v3dlib_image)

target_compile_options(v3dlib_render PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
