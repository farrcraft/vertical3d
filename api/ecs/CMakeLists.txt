v3d_add_api_library(ecs
	"component/PositionFixed2D.h" "component/PositionFixed2D.cpp"
 "component/Position2D.h" "component/Position2D.cpp"
 "component/Position1D.h" "component/Position1D.cpp"
 "component/Color3.h" "component/Color3.cpp"
 "System.h" "System.cpp")

target_link_libraries(v3dlib_ecs PUBLIC glm::glm EnTT::EnTT)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
