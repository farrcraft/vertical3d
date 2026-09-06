v3d_add_api_library(render
	"realtime/Frame.h" "realtime/Frame.cpp"
	"realtime/Engine.h" "realtime/Engine.cpp" 
	"realtime/Context.h" "realtime/Window.h" "realtime/Window.cpp" 
	"realtime/Engine3D.h" "realtime/Engine3D.cpp" 
	"realtime/Context3D.h" "realtime/Context3D.cpp" 
	"realtime/Handle.h" "realtime/Registry.h"
	"realtime/Canvas.h" "realtime/Canvas.cpp"
	"realtime/LineCanvas.h" "realtime/LineCanvas.cpp"
	"realtime/DrawItem.h" "realtime/DrawItem.cpp"
	"realtime/Pass.h" "realtime/Pass.cpp"
	"realtime/vulkan/Result.h" "realtime/vulkan/Result.cxx" 
	"realtime/vulkan/Instance.h" "realtime/vulkan/Instance.cxx" 
	"realtime/vulkan/Surface.h" "realtime/vulkan/Surface.cxx" 
	"realtime/vulkan/Device.h" "realtime/vulkan/Device.cxx" 
	"realtime/vulkan/Swapchain.h" "realtime/vulkan/Swapchain.cxx"
	"realtime/vulkan/CommandPool.h" "realtime/vulkan/CommandPool.cxx"
	"realtime/vulkan/PipelineCache.h" "realtime/vulkan/PipelineCache.cxx"
	"realtime/vulkan/Presenter.h" "realtime/vulkan/Presenter.cxx"
	"realtime/vulkan/Recorder.h" "realtime/vulkan/Recorder.cxx"
	"realtime/vulkan/Resources.h" "realtime/vulkan/Resources.cxx"
	"realtime/vulkan/Memory.h" "realtime/vulkan/Memory.cxx"
	"realtime/vulkan/Buffer.h" "realtime/vulkan/Buffer.cxx"
	"realtime/vulkan/Uploader.h" "realtime/vulkan/Uploader.cxx"
	"realtime/vulkan/DeviceBuffer.h" "realtime/vulkan/DeviceBuffer.cxx"
	"realtime/vulkan/Mesh.h" "realtime/vulkan/Mesh.cxx"
	"realtime/vulkan/DepthBuffer.h" "realtime/vulkan/DepthBuffer.cxx"
	"realtime/vulkan/FrameUniforms.h" "realtime/vulkan/FrameUniforms.cxx"
	"realtime/vulkan/PipelineBuilder.h" "realtime/vulkan/PipelineBuilder.cxx"
	"realtime/vulkan/TextureFactory.h" "realtime/vulkan/TextureFactory.cxx"
	"realtime/vulkan/QuadRenderer.h" "realtime/vulkan/QuadRenderer.cxx"
	"realtime/vulkan/LineRenderer.h" "realtime/vulkan/LineRenderer.cxx")

# The one batched quad primitive of ADR-0005. Compiled to SPIR-V and embedded rather than
# loaded from disk - see v3d_add_shader in the root CMakeLists.
v3d_add_shader(v3dlib_render "shaders/quad.vert")
v3d_add_shader(v3dlib_render "shaders/quad.frag")

# The line primitive of ADR-0011.
v3d_add_shader(v3dlib_render "shaders/line.vert")
v3d_add_shader(v3dlib_render "shaders/line.frag")

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

# offline is added below the flags above for the same reason tests is - a subdirectory added
# over them does not inherit /utf-8, and spdlog's bundled fmt has a static_assert that fails
# without it.
add_subdirectory("offline")

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
