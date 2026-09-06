v3d_add_api_library(dag
"Edge.h" "Edge.cxx"
"Group.h" "Group.cxx"
"Node.h" "Node.cxx"
"Root.h" "Root.cxx"
"Set.h" "Set.cxx"
"State.h" "State.cxx"
"Switch.h" "Switch.cxx"
"Transform.h" "Transform.cxx"
"View.h" "View.cxx"
)

target_link_libraries(v3dlib_dag PUBLIC glm::glm)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
