v3d_add_api_library(audio "AudioClip.h" "AudioClip.cpp" "Engine.h" "Engine.cpp" "Play.h" "Play.cpp"
	"Loaders.h" "Loaders.cpp" "kind/Sound.h" "kind/Sound.cpp" "loader/Wav.h" "loader/Wav.cpp")

# PUBLIC: these sources compile Logger.h, and so do their consumers.
# EnTT: Engine.h holds a registry.
target_link_libraries(v3dlib_audio PUBLIC v3dlib_log EnTT::EnTT)

# PRIVATE: AudioClip.h declares the MIX_ handles it names rather than including the header
# that defines them, so only these sources compile SDL_mixer.h.
target_link_libraries(v3dlib_audio PRIVATE SDL3_mixer::SDL3_mixer)

# Engine.h names an event::Sound, and the boost::json::object a sound config is.
# v3dlib_asset: the wav loader and the sound it builds are an asset::Loader and an
# asset::Asset - ADR-0079. The edge runs from here to the asset core and not back.
target_link_libraries(v3dlib_audio PUBLIC v3dlib_event v3dlib_asset Boost::json)

target_compile_options(v3dlib_audio PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
