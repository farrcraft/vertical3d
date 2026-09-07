v3d_add_api_library(grid
"TileGrid.h" "TileGrid.cpp"
"Pathfinding.h" "Pathfinding.cpp"
"LineOfSight.h" "LineOfSight.cpp"
"Overlay.h" "Overlay.cpp"
)

target_link_libraries(v3dlib_grid PUBLIC glm::glm)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
