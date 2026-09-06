v3d_add_api_library(log
	"Logger.h" "Logger.cpp"
)

# spdlog is a PUBLIC dependency, not an implementation detail: Logger.h exposes spdlog
# types, and the spdlog::spdlog target carries SPDLOG_COMPILED_LIB in its interface. Any
# target that compiles Logger.h without that definition builds spdlog header-only and
# emits symbols that the compiled spdlog library also defines, which only shows up as a
# duplicate symbol at link time in whichever app happens to pull the wrong object first.
target_link_libraries(v3dlib_log PUBLIC spdlog::spdlog)

target_compile_options(v3dlib_log PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
