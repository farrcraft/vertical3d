v3d_add_api_library(audio "AudioClip.h" "AudioClip.cpp" "Engine.h" "Engine.cpp" "Play.h" "Play.cpp"
	"Loaders.h" "Loaders.cpp" "kind/Sound.h" "kind/Sound.cpp" "loader/Wav.h" "loader/Wav.cpp"
	"Events.h" "Events.cpp" "NullEvents.h" "NullEvents.cpp")

# The FMOD Studio event backend, built only against an SDK installed on this machine. The SDK's
# licence forbids committing it, and its downloads need an account, so nothing here fetches it.
# Empty, the default, builds the null event backend alone, as CI does.
set(V3D_FMOD_ROOT "" CACHE PATH "An FMOD Studio API for Windows install, to build the FMOD event backend against")
if(V3D_FMOD_ROOT)
	if(NOT EXISTS "${V3D_FMOD_ROOT}/api/studio/inc/fmod_studio.hpp")
		message(FATAL_ERROR "V3D_FMOD_ROOT is ${V3D_FMOD_ROOT}, which holds no api/studio/inc/fmod_studio.hpp")
	endif()
	add_library(v3d_fmod_core SHARED IMPORTED GLOBAL)
	set_target_properties(v3d_fmod_core PROPERTIES
		IMPORTED_LOCATION "${V3D_FMOD_ROOT}/api/core/lib/x64/fmod.dll"
		IMPORTED_IMPLIB "${V3D_FMOD_ROOT}/api/core/lib/x64/fmod_vc.lib"
		INTERFACE_INCLUDE_DIRECTORIES "${V3D_FMOD_ROOT}/api/core/inc")
	add_library(v3d_fmod_studio SHARED IMPORTED GLOBAL)
	set_target_properties(v3d_fmod_studio PROPERTIES
		IMPORTED_LOCATION "${V3D_FMOD_ROOT}/api/studio/lib/x64/fmodstudio.dll"
		IMPORTED_IMPLIB "${V3D_FMOD_ROOT}/api/studio/lib/x64/fmodstudio_vc.lib"
		INTERFACE_INCLUDE_DIRECTORIES "${V3D_FMOD_ROOT}/api/studio/inc"
		INTERFACE_LINK_LIBRARIES v3d_fmod_core)
	target_sources(v3dlib_audio PRIVATE "FmodEvents.h" "FmodEvents.cpp")
	target_link_libraries(v3dlib_audio PRIVATE v3d_fmod_studio)
	target_compile_definitions(v3dlib_audio PRIVATE V3D_AUDIO_FMOD)
endif()

# PUBLIC: these sources compile Logger.h, and so do their consumers.
# EnTT: Engine.h holds a dispatcher and a scoped_connection to it.
target_link_libraries(v3dlib_audio PUBLIC v3dlib_log EnTT::EnTT)

# PRIVATE: AudioClip.h declares the MIX_ handles it names rather than including the header
# that defines them, so only these sources compile SDL_mixer.h.
target_link_libraries(v3dlib_audio PRIVATE SDL3_mixer::SDL3_mixer)

# Engine.h names an event::Sound, and the boost::json::object a sound config is.
# v3dlib_asset: the wav loader and the sound it builds are an asset::Loader and an
# asset::Asset. This library depends on the asset core, and not the other way round.
target_link_libraries(v3dlib_audio PUBLIC v3dlib_event v3dlib_asset Boost::json)

target_compile_options(v3dlib_audio PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
