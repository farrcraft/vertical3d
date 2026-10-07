v3d_add_api_library(dag "Node.h" "Node.cxx" "Transform.h" "Transform.cxx")

target_link_libraries(v3dlib_dag PUBLIC v3dlib_type glm::glm)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
