add_library(v3dlib_audio "AudioClip.h" "AudioClip.cpp" "Engine.h" "Engine.cpp")

# PUBLIC: these sources compile Logger.h, and so do their consumers.
# The mixer is PUBLIC for the same reason - AudioClip and Engine name MIX_ types in their
# headers, so anything compiling them needs its include directory.
target_link_libraries(v3dlib_audio PUBLIC v3dlib_log SDL3_mixer::SDL3_mixer)

target_compile_options(v3dlib_audio PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")

add_subdirectory("tests")
