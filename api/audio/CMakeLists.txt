add_library(v3dlib_audio "AudioClip.h" "AudioClip.cpp" "Engine.h" "Engine.cpp")

target_compile_options(v3dlib_audio PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")
