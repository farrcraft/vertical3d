v3d_add_api_library(log
	"Logger.h" "Logger.cpp"
)

# spdlog is a PUBLIC dependency because Logger.h exposes spdlog types. The spdlog::spdlog
# target carries SPDLOG_COMPILED_LIB in its interface. A target that compiles Logger.h
# without that definition builds spdlog header-only. Its objects then define symbols that the
# compiled spdlog library also defines, and the app fails to link with a duplicate symbol.
target_link_libraries(v3dlib_log PUBLIC spdlog::spdlog)

# Every narrow path in the api is UTF-8. Windows reads a narrow path as UTF-8 only in a process
# whose manifest sets the UTF-8 code page. Every executable links the logger, so the logger
# carries the manifest, and the linker embeds it in each executable that links this library.
# Other platforms read a narrow path as UTF-8 already, and have no manifest.
if(WIN32)
	target_sources(v3dlib_log INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/Utf8.manifest")
endif()

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
