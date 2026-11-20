# RIB, the RenderMan Interface Bytestream: its lexer, reader, writer and recorder, and the
# requests a handler takes. It names no renderer, so the editor writes RIB without linking moya.
# It links neither Vulkan nor SDL, and its suite runs in CI on a runner with no GPU.

v3d_add_api_library(rib
	"source/Characters.h" "source/Characters.cxx"
	"source/Lexeme.h"
	"source/Position.h"
	"Token.h"
	"Lexer.h" "Lexer.cxx"
	"Words.h" "Words.cxx"
	"Declaration.h" "Declaration.cxx"
	"Declarations.h" "Declarations.cxx"
	"Parameters.h" "Parameters.cxx"
	"Sampling.h" "Sampling.cxx"
	"Bases.h" "Bases.cxx"
	"requests/Basis.h"
	"requests/Clipping.h"
	"requests/Cone.h"
	"requests/CropWindow.h"
	"requests/Curves.h"
	"requests/Cylinder.h"
	"requests/Declare.h"
	"requests/DepthOfField.h"
	"requests/Disk.h"
	"requests/Display.h"
	"requests/DisplayChannel.h"
	"requests/Format.h"
	"requests/Hyperboloid.h"
	"requests/LightSource.h"
	"requests/MakeTexture.h"
	"requests/Paraboloid.h"
	"requests/PointsPolygons.h"
	"requests/ScreenWindow.h"
	"requests/ShadingNode.h"
	"requests/Shutter.h"
	"requests/Sphere.h"
	"requests/SubdivisionMesh.h"
	"requests/Torus.h"
	"Handler.h"
	"internal/Operands.h" "internal/Operands.cxx"
	"internal/Primitives.h" "internal/Primitives.cxx"
	"Reader.h" "Reader.cxx"
	"Recorder.h" "Recorder.cxx"
	"Writer.h" "Writer.cxx"
)

# PUBLIC: the reader and its handlers log through Logger.h, and the requests name glm types.
target_link_libraries(v3dlib_rib PUBLIC v3dlib_log glm::glm)

# PRIVATE: a count read from a number is checked through api/type, in sources alone.
target_link_libraries(v3dlib_rib PRIVATE v3dlib_type)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
