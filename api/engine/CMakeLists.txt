v3d_add_api_library(engine
	"Application.h" "Application.cpp"
	"Feature.h"
	"Accumulator.h" "Accumulator.cpp"
	"Statistics.h" "Statistics.cpp"
	"Engine.h" "Engine.cpp"
	"Settings.h" "Settings.cpp"
	"Main.h"
	"IsometricController.h" "IsometricController.cpp"
)

# PUBLIC: Engine.h holds one of each of these, so an app that subclasses it compiles all of
# them - the logger, the asset manager, the config, the event and input engines, the window,
# and the registry. type, because Feature.h is a type::Flags. SDL3 because it routes each
# SDL_Event and hands it to the app. Boost::json because Engine.h and Settings.h name
# boost::json types.
target_link_libraries(v3dlib_engine PUBLIC
	v3dlib_log
	v3dlib_asset
	v3dlib_config
	v3dlib_event
	v3dlib_input
	v3dlib_render
	v3dlib_type
	SDL3::SDL3
	EnTT::EnTT
	Boost::json)

# PRIVATE: initialize() registers the media loaders on the asset manager it builds and
# resolves the config path, and appPath() derives a path from argv[0].
# No header here includes boost::filesystem: the run() template calls appPath() instead, so an
# app that includes Application.h does not need it.
target_link_libraries(v3dlib_engine PRIVATE Boost::filesystem v3dlib_asset_media)

target_compile_options(v3dlib_engine PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
