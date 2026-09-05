# The offline renderers of ADR-0022 share this library. It names neither Vulkan nor SDL:
# talyn and moya touch no window, no device and no swapchain, which is what lets their
# suites run in CI where everything below the recorder in api/render cannot.
add_library("v3dlib_render_offline"
	"FrameBuffer.h" "FrameBuffer.cxx"
)

# Workaround for a boost log ABI bug - https://github.com/microsoft/vcpkg/discussions/22762
target_compile_definitions(v3dlib_render_offline PUBLIC BOOST_USE_WINAPI_VERSION=0x0A00)
target_compile_definitions(v3dlib_render_offline PUBLIC _WIN32_WINNT=0x0A00)
target_compile_definitions(v3dlib_render_offline PUBLIC WINVER=0x0A00)

# PUBLIC: these sources compile Logger.h, and so do their consumers.
target_link_libraries(v3dlib_render_offline PUBLIC v3dlib_log v3dlib_image v3dlib_type)

target_compile_options(v3dlib_render_offline PRIVATE /EHsc)

add_subdirectory("tests")
