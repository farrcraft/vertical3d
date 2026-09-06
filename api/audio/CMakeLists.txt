v3d_add_api_library(audio "AudioClip.h" "AudioClip.cpp" "Engine.h" "Engine.cpp")

# PUBLIC: these sources compile Logger.h, and so do their consumers.
# The mixer is PUBLIC for the same reason - AudioClip and Engine name MIX_ types in their
# headers, so anything compiling them needs its include directory.
# EnTT: Engine.h holds a registry.
target_link_libraries(v3dlib_audio PUBLIC v3dlib_log SDL3_mixer::SDL3_mixer EnTT::EnTT)

# Engine.h names an asset::Json and an event::Sound. The asset edge closes a cycle -
# asset's Wav loader builds an audio clip - which CMake resolves for static libraries by
# repeating them on the link line. Breaking it would mean moving one of the two calls.
target_link_libraries(v3dlib_audio PUBLIC v3dlib_asset v3dlib_event)

target_link_libraries(v3dlib_audio PRIVATE Boost::json)

target_compile_options(v3dlib_audio PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
