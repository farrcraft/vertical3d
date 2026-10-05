v3d_add_api_library(audio "AudioClip.h" "AudioClip.cpp" "Engine.h" "Engine.cpp" "Play.h" "Play.cpp")

# PUBLIC: these sources compile Logger.h, and so do their consumers.
# The mixer is PUBLIC for the same reason - AudioClip and Engine name MIX_ types in their
# headers, so anything compiling them needs its include directory.
# EnTT: Engine.h holds a registry.
target_link_libraries(v3dlib_audio PUBLIC v3dlib_log SDL3_mixer::SDL3_mixer EnTT::EnTT)

# Engine.h names an event::Sound, and the boost::json::object a sound config is. It names no
# asset type: asset's Wav loader builds an audio clip, so the edge runs one way only - ADR-0021.
target_link_libraries(v3dlib_audio PUBLIC v3dlib_event Boost::json)

target_compile_options(v3dlib_audio PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
