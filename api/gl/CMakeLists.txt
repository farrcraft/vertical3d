add_library("v3dlib_gl"
	"Canvas.h" "Canvas.cxx"
	"GLTexture.h" "GLTexture.cxx"
	"Program.h" "Program.cxx"
	"Shader.h" "Shader.cxx"  
	"VertexBuffer.h" "VertexBuffer.cxx"
)

# Workaround for a boost log ABI bug - https://github.com/microsoft/vcpkg/discussions/22762
target_compile_definitions(v3dlib_gl PUBLIC BOOST_USE_WINAPI_VERSION=0x0A00)
target_compile_definitions(v3dlib_gl PUBLIC _WIN32_WINNT=0x0A00)
target_compile_definitions(v3dlib_gl PUBLIC WINVER=0x0A00)
