add_library(v3dlib_audio "AudioClip.h" "AudioClip.cpp" "Engine.h" "Engine.cpp")

# PUBLIC: these sources compile Logger.h, and so do their consumers.
target_link_libraries(v3dlib_audio PUBLIC v3dlib_log)

target_compile_options(v3dlib_audio PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")
