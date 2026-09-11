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
	"rib/Lexer.h" "rib/Lexer.cxx"
	"rib/Declarations.h" "rib/Declarations.cxx"
	"rib/Parameters.h" "rib/Parameters.cxx"
	"rib/Arguments.h" "rib/Arguments.cxx"
	"rib/Handler.h"
	"rib/Reader.h" "rib/Reader.cxx"
	"sl/Lexer.h" "sl/Lexer.cxx"
	"sl/Syntax.h" "sl/Syntax.cxx"
	"sl/Parser.h" "sl/Parser.cxx"
	"sl/Types.h" "sl/Types.cxx"
	"sl/Builtins.h" "sl/Builtins.cxx"
	"sl/Compiler.h" "sl/Compiler.cxx"
	"sl/runtime/Value.h" "sl/runtime/Value.cxx"
	"sl/runtime/Program.h" "sl/runtime/Program.cxx"
	"sl/runtime/Renderer.h" "sl/runtime/Renderer.cxx"
	"sl/Emitter.h" "sl/Emitter.cxx"
	"sl/runtime/Machine.h" "sl/runtime/Machine.cxx"
	"sl/runtime/Library.cxx"
	"sl/Instance.h" "sl/Instance.cxx"
	"sl/ShaderLibrary.h" "sl/ShaderLibrary.cxx"
	"sl/Imager.h" "sl/Imager.cxx"
)

# PUBLIC: these sources compile Logger.h, and so do their consumers.
target_link_libraries(v3dlib_render_offline PUBLIC v3dlib_log v3dlib_image v3dlib_type glm::glm)

target_compile_options(v3dlib_render_offline PRIVATE /EHsc)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
