v3d_add_api_library(grid
"TileCoord.h"
"TileGrid.h" "TileGrid.cpp"
"TileFilter.h"
"ReachableTile.h"
"DistanceField.h"
"Pathfinding.h" "Pathfinding.cpp"
"LineOfSight.h" "LineOfSight.cpp"
"Overlay.h" "Overlay.cpp"
"Picture.h" "Picture.cpp"
"Terrain.h"
"Unknown.h"
)

target_link_libraries(v3dlib_grid PUBLIC glm::glm)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
