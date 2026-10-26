v3d_add_api_library(engine
	"Application.h" "Application.cpp"
	"Feature.h"
	"Accumulator.h" "Accumulator.cpp"
	"Statistics.h" "Statistics.cpp"
	"Engine.h" "Engine.cpp"
	"Identity.h"
	"Settings.h" "Settings.cpp"
	"Main.h"
	"IsometricController.h" "IsometricController.cpp"
	"CameraProfiles.h" "CameraProfiles.cpp"
	"WindowSize.h" "WindowSize.cpp"
)

# PUBLIC: Engine.h holds one of each of these, so an app that subclasses it compiles all of
# them. They are the logger, the asset manager and its manifest, the event router, the input
# devices, the window, and the registry. ecs, because interpolate<T>() in Engine.h snapshots
# through ecs::Previous. type, because Feature.h is a type::Flags and CameraProfiles.h hands out
# a type::camera::Profile. SDL3, because Engine.h passes each SDL_Event to the app. Boost::json,
# because Engine.h, Settings.h and the document readers name boost::json types. glm, because
# WindowSize.h hands out a size as an ivec2.
target_link_libraries(v3dlib_engine PUBLIC
	v3dlib_log
	v3dlib_asset
	v3dlib_ecs
	v3dlib_event
	v3dlib_input
	v3dlib_window
	v3dlib_type
	SDL3::SDL3
	EnTT::EnTT
	Boost::json
	glm::glm)

# PRIVATE: initialize() registers the media loaders on the asset manager it builds, and
# appPath() derives a path from argv[0] with boost::filesystem.
# No header here includes boost::filesystem. The run() template calls appPath(), so an app that
# includes Application.h does not need it.
target_link_libraries(v3dlib_engine PRIVATE Boost::filesystem v3dlib_asset_media)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
