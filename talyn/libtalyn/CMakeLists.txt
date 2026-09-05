add_library("v3dlib_talyn"
	"RenderContext.h" "RenderContext.cxx"
	"RIBReader.h" "RIBReader.cxx"
	"Scene.h" "Scene.cxx"
)

# Workaround for a boost log ABI bug - https://github.com/microsoft/vcpkg/discussions/22762
target_compile_definitions(v3dlib_talyn PUBLIC BOOST_USE_WINAPI_VERSION=0x0A00)
target_compile_definitions(v3dlib_talyn PUBLIC _WIN32_WINNT=0x0A00)
target_compile_definitions(v3dlib_talyn PUBLIC WINVER=0x0A00)

# PUBLIC: the framebuffer is part of the render context's interface, and the camera and the
# ray are part of the scene's.
target_link_libraries(v3dlib_talyn PUBLIC v3dlib_render_offline v3dlib_type)

target_compile_options(v3dlib_talyn PRIVATE /EHsc /utf-8)
