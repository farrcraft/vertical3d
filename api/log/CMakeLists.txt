add_library("v3dlib_log"
	"Logger.h" "Logger.cpp"
)

# Workaround for a boost log ABI bug - https://github.com/microsoft/vcpkg/discussions/22762
target_compile_definitions(v3dlib_log PUBLIC BOOST_USE_WINAPI_VERSION=0x0A00)
target_compile_definitions(v3dlib_log PUBLIC _WIN32_WINNT=0x0A00)
target_compile_definitions(v3dlib_log PUBLIC WINVER=0x0A00)

# spdlog is a PUBLIC dependency, not an implementation detail: Logger.h exposes spdlog
# types, and the spdlog::spdlog target carries SPDLOG_COMPILED_LIB in its interface. Any
# target that compiles Logger.h without that definition builds spdlog header-only and
# emits symbols that the compiled spdlog library also defines, which only shows up as a
# duplicate symbol at link time in whichever app happens to pull the wrong object first.
target_link_libraries(v3dlib_log PUBLIC spdlog::spdlog)

target_compile_options(v3dlib_log PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")
