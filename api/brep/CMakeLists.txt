add_library("v3dlib_brep" "BRep.h" "BRep.cxx" "Face.h" "Face.cxx" "HalfEdge.h" "HalfEdge.cxx" "Vertex.h" "Vertex.cxx")
target_link_libraries("v3dlib_brep" PUBLIC v3dlib_dag)
add_subdirectory("tests")
