add_executable("talyn"
	"src/FrameBuffer.h" "src/Framebuffer.cxx"
	"src/RenderContext.h" "src/RenderContext.cxx"
	"src/RIBReader.h" "src/RIBReader.cxx"
	  "src/talyn.cxx")

target_link_libraries("talyn" PRIVATE
	v3dlib_log
	v3dlib_image
	${Boost_LIBRARIES}
	#"turbojpeg-static"
	#"jpeg-static"
	#"libpng16_static"
	#"zlib"
	${PNG_LIBRARIES}
	JPEG::JPEG
	Boost::log
	${SDL2_LIBRARIES}
)

# Workaround for a boost log ABI bug - https://github.com/microsoft/vcpkg/discussions/22762
target_compile_definitions(talyn PUBLIC BOOST_USE_WINAPI_VERSION=0x0A00)
target_compile_definitions(talyn PUBLIC _WIN32_WINNT=0x0A00)
target_compile_definitions(talyn PUBLIC WINVER=0x0A00)
