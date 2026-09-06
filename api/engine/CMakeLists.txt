v3d_add_api_library(engine
	"Feature.h"
	"Engine.h" "Engine.cpp"
)

# PUBLIC: Engine.h holds one of each of these, so an app that subclasses it compiles all of
# them - the logger, the asset manager, the config, the event and input engines, the window,
# and the registry it passes to the render engine.
target_link_libraries(v3dlib_engine PUBLIC
	v3dlib_log
	v3dlib_asset
	v3dlib_config
	v3dlib_event
	v3dlib_input
	v3dlib_render
	EnTT::EnTT)

# PRIVATE: initialize() opens the window and resolves the config path, and neither SDL nor
# boost::filesystem is named by Engine.h.
target_link_libraries(v3dlib_engine PRIVATE SDL3::SDL3 Boost::filesystem)

target_compile_options(v3dlib_engine PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
