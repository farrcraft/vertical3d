add_library("v3dlib_render"
	"realtime/2D/Context2D.h" "realtime/2D/Context2D.cpp"
	"realtime/Surface.h" "realtime/Surface.cpp"
	"realtime/Frame.h" "realtime/Frame.cpp"
	"realtime/Operation.h"
	"realtime/2D/Texture2D.h" "realtime/2D/Texture2D.cpp"
	"realtime/operation/2D/Blit2DTexture.h" "realtime/operation/2D/Blit2DTexture.cpp"
	"realtime/2D/Texture2DCache.h" "realtime/2D/Texture2DCache.cpp"
	"realtime/operation/GLTexturedQuad.h" "realtime/operation/GLTexturedQuad.cpp"
	"realtime/operation/GLTexture.h" "realtime/operation/GLTexture.cpp"
	"realtime/operation/Overlay.h" "realtime/operation/Overlay.cpp"
	"realtime/operation/GLFont.h" "realtime/operation/GLFont.cpp"
	"realtime/operation/TextureFont.h" "realtime/operation/TextureFont.cpp"
	"realtime/operation/BitmapFont.h" "realtime/operation/BitmapFont.cpp" 
	"realtime/2D/Engine2D.h" "realtime/2D/Engine2D.cpp" 
	"realtime/2D/Window2D.h" "realtime/2D/Window2D.cpp"
	"realtime/2D/Scene2D.h" "realtime/2D/Scene2D.cpp" 
	"realtime/Engine.h" "realtime/Engine.cpp" 
	"realtime/Context.h" "realtime/Window.h" "realtime/Window.cpp" 
	"realtime/2D/Operation2D.h" "realtime/2D/Operation2D.cpp" 
	"realtime/Window3D.h" "realtime/Window3D.cpp" 
	"realtime/Engine3D.h" "realtime/Engine3D.cpp" 
	"realtime/operation/Canvas.h" "realtime/operation/Canvas.cpp" 
	"realtime/Context3D.h" "realtime/Context3D.cpp" 
	"realtime/Handle.h" "realtime/Registry.h"
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
	"realtime/vulkan/Resources.h" "realtime/vulkan/Resources.cxx")

# Workaround for a boost log ABI bug - https://github.com/microsoft/vcpkg/discussions/22762
target_compile_definitions(v3dlib_render PUBLIC BOOST_USE_WINAPI_VERSION=0x0A00)
target_compile_definitions(v3dlib_render PUBLIC _WIN32_WINNT=0x0A00)
target_compile_definitions(v3dlib_render PUBLIC WINVER=0x0A00)

# PUBLIC: these sources compile Logger.h, and so do their consumers.
target_link_libraries(v3dlib_render PUBLIC v3dlib_log)

target_compile_options(v3dlib_render PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")

add_subdirectory("tests")
