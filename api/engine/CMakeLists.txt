v3d_add_api_library(engine
	"Application.h" "Application.cpp"
	"Feature.h"
	"Accumulator.h" "Accumulator.cpp"
	"Statistics.h" "Statistics.cpp"
	"Engine.h" "Engine.cpp"
	"Settings.h" "Settings.cpp"
)

# PUBLIC: Engine.h holds one of each of these, so an app that subclasses it compiles all of
# them - the logger, the asset manager, the config, the event and input engines, the window,
# and the registry. SDL3 because it routes each SDL_Event and hands it to the app.
target_link_libraries(v3dlib_engine PUBLIC
	v3dlib_log
	v3dlib_asset
	v3dlib_config
	v3dlib_event
	v3dlib_input
	v3dlib_render
	SDL3::SDL3
	EnTT::EnTT)

# PRIVATE: initialize() resolves the config path, and appPath() derives one from argv[0].
# boost::filesystem is named by no header here - the run() template calls appPath() rather
# than reaching for the filesystem itself, so an app that includes Application.h does not
# need it either.
target_link_libraries(v3dlib_engine PRIVATE Boost::filesystem)

target_compile_options(v3dlib_engine PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
