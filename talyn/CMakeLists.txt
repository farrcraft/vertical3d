add_executable("talyn" "src/FrameBuffer.h" "src/Framebuffer.cxx" "src/RenderContext.h" "src/RenderContext.cxx" "src/RIBReader.h" "src/RIBReader.cxx"
	"src/XMLSceneReader.h" "src/XMLSceneReader.cxx" "src/talyn.cxx")

target_link_libraries("talyn" 
	v3dlib_image
	${Boost_LIBRARIES}
	"turbojpeg-static" "jpeg-static"
	"libpng16d" "zlibd")