# The offline renderers of ADR-0022 share this library. It names neither Vulkan nor SDL:
# talyn and moya touch no window, no device and no swapchain, which is what lets their
# suites run in CI where everything below the recorder in api/render cannot - and what makes
# it selectable without the realtime stack, per ADR-0033.
#
# /utf-8 is set here rather than inherited: this directory is added by api/CMakeLists.txt
# rather than by the api/render one whose flags used to reach it, and spdlog's bundled fmt
# has a static_assert that fails without it.
set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")

v3d_add_api_library(render_offline
	"FrameBuffer.h" "FrameBuffer.cxx"
	"RIBLexer.h" "RIBLexer.cxx"
	"RIBDeclarations.h" "RIBDeclarations.cxx"
	"RIBParameters.h" "RIBParameters.cxx"
	"RIBHandler.h"
	"RIBReader.h" "RIBReader.cxx"
)

# PUBLIC: these sources compile Logger.h, and so do their consumers.
target_link_libraries(v3dlib_render_offline PUBLIC v3dlib_log v3dlib_image v3dlib_type glm::glm)

target_compile_options(v3dlib_render_offline PRIVATE /EHsc)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
