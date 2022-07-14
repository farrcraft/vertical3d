add_executable("vertical3d"
	"CameraControlTool.h" "CameraControlTool.cxx"
	"Controller.h" "Controller.cxx"
	"HWRenderContext.h" "HWRenderContext.cxx"
	"Tool.h"
	"ViewPort.h" "ViewPort.cxx"
	"Visitor.h"
	"vertical3d.cxx")

target_link_libraries(vertical3d
	v3dlib_core
	v3dlib_image
	${Boost_LIBRARIES}
	"turbojpeg-static"
	"jpeg-static"
	"libpng16d"
	"zlibd")
