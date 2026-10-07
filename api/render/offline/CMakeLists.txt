# The code shared by the offline renderer, moya. This library must not link Vulkan or SDL.
# It touches no window, device or swapchain, so its suite and moya's run in CI on a runner
# with no GPU. A consumer can also select it without the realtime stack.
#
# /utf-8 is set here because api/CMakeLists.txt adds this directory, so it does not inherit
# api/render's flags. spdlog's bundled fmt has a static_assert that fails without it.
set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")

v3d_add_api_library(render_offline
	"Characters.h" "Characters.cxx"
	"Lexeme.h"
	"FrameBuffer.h" "FrameBuffer.cxx"
	"Sampling.h" "Sampling.cxx"
	"Sampler.h" "Sampler.cxx"
	"Film.h" "Film.cxx"
	"MovingTransform.h" "MovingTransform.cxx"
	"SearchPath.h" "SearchPath.cxx"
	"Texture.h" "Texture.cxx"
	"Textures.h" "Textures.cxx"
	"Noise.h" "Noise.cxx"
	"rib/Token.h"
	"rib/Lexer.h" "rib/Lexer.cxx"
	"rib/Words.h" "rib/Words.cxx"
	"rib/Declaration.h" "rib/Declaration.cxx"
	"rib/Declarations.h" "rib/Declarations.cxx"
	"rib/Parameters.h" "rib/Parameters.cxx"
	"rib/Arguments.h" "rib/Arguments.cxx"
	"rib/Handler.h"
	"rib/Reader.h" "rib/Reader.cxx"
	"sl/Token.h"
	"sl/Lexer.h" "sl/Lexer.cxx"
	"sl/syntax/Expression.h" "sl/syntax/Expression.cxx"
	"sl/syntax/Number.h" "sl/syntax/Number.cxx"
	"sl/syntax/String.h" "sl/syntax/String.cxx"
	"sl/syntax/Variable.h" "sl/syntax/Variable.cxx"
	"sl/syntax/Call.h" "sl/syntax/Call.cxx"
	"sl/syntax/Unary.h" "sl/syntax/Unary.cxx"
	"sl/syntax/Binary.h" "sl/syntax/Binary.cxx"
	"sl/syntax/Ternary.h" "sl/syntax/Ternary.cxx"
	"sl/syntax/Cast.h" "sl/syntax/Cast.cxx"
	"sl/syntax/Tuple.h" "sl/syntax/Tuple.cxx"
	"sl/syntax/Index.h" "sl/syntax/Index.cxx"
	"sl/syntax/Statement.h" "sl/syntax/Statement.cxx"
	"sl/syntax/Block.h" "sl/syntax/Block.cxx"
	"sl/syntax/Declarator.h"
	"sl/syntax/Declaration.h" "sl/syntax/Declaration.cxx"
	"sl/syntax/Assignment.h" "sl/syntax/Assignment.cxx"
	"sl/syntax/Conditional.h" "sl/syntax/Conditional.cxx"
	"sl/syntax/While.h" "sl/syntax/While.cxx"
	"sl/syntax/For.h" "sl/syntax/For.cxx"
	"sl/syntax/Jump.h" "sl/syntax/Jump.cxx"
	"sl/syntax/ExpressionStatement.h" "sl/syntax/ExpressionStatement.cxx"
	"sl/syntax/Lighting.h" "sl/syntax/Lighting.cxx"
	"sl/syntax/Parameter.h"
	"sl/syntax/Function.h"
	"sl/syntax/Shader.h" "sl/syntax/Shader.cxx"
	"sl/syntax/Walk.h" "sl/syntax/Walk.cxx"
	"sl/Parser.h" "sl/Parser.cxx"
	"sl/Types.h" "sl/Types.cxx"
	"sl/Builtins.h" "sl/Builtins.cxx"
	"sl/Symbol.h"
	"sl/Compiler.h" "sl/Compiler.cxx"
	"sl/Inference.h" "sl/Inference.cxx"
	"sl/runtime/Value.h" "sl/runtime/Value.cxx"
	"sl/runtime/Instruction.h"
	"sl/runtime/Register.h"
	"sl/runtime/Program.h" "sl/runtime/Program.cxx"
	"sl/runtime/Renderer.h" "sl/runtime/Renderer.cxx"
	"sl/Emitter.h" "sl/Emitter.cxx"
	"sl/runtime/Machine.h" "sl/runtime/Machine.cxx"
	"sl/runtime/Library.cxx"
	"sl/Instance.h" "sl/Instance.cxx"
	"sl/Placed.h"
	"sl/Globals.h" "sl/Globals.cxx"
	"sl/ShaderLibrary.h" "sl/ShaderLibrary.cxx"
	"sl/Imager.h" "sl/Imager.cxx"
	"trace/Pose.h"
	"trace/Intersection.h"
	"trace/Primitive.h" "trace/Primitive.cxx"
	"trace/Triangle.h" "trace/Triangle.cxx"
	"trace/Sphere.h" "trace/Sphere.cxx"
	"trace/Hit.h"
	"trace/Scene.h" "trace/Scene.cxx"
	"trace/HitShader.h" "trace/HitShader.cxx"
	"trace/Tracer.h" "trace/Tracer.cxx"
)

# PUBLIC: these sources compile Logger.h, and so do their consumers.
target_link_libraries(v3dlib_render_offline PUBLIC v3dlib_log v3dlib_image v3dlib_type glm::glm)

# PRIVATE: Textures.cxx searches for a texture file with boost::filesystem, and no header here
# includes it.
target_link_libraries(v3dlib_render_offline PRIVATE Boost::filesystem)

target_compile_options(v3dlib_render_offline PRIVATE /EHsc)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
