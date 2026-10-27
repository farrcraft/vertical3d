v3d_add_api_library(brep "BRep.h" "BRep.cxx"
	"Topology.h" "Topology.cxx" "Operations.h" "Operations.cxx" "Face.h" "Face.cxx" "HalfEdge.h" "HalfEdge.cxx" "Vertex.h" "Vertex.cxx" "Index.h"
	"Primitives.h" "Primitives.cxx" "Triangulation.h" "Triangulation.cxx")

target_link_libraries(v3dlib_brep PUBLIC v3dlib_type glm::glm)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
