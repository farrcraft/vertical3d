add_library("v3dlib_brep" "BRep.h" "BRep.cxx" "Face.h" "Face.cxx" "HalfEdge.h" "HalfEdge.cxx" "Vertex.h" "Vertex.cxx"
	"Index.h" "Edge.h" "Edge.cxx" "WingedEdgeBRep.h" "WingedEdgeBRep.cxx")
target_link_libraries("v3dlib_brep" PUBLIC v3dlib_dag)
add_subdirectory("tests")
