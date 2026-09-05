add_executable("talyn" "talyn.cxx")

target_link_libraries("talyn" PRIVATE
	v3dlib_talyn
	v3dlib_image
	${Boost_LIBRARIES}
	${PNG_LIBRARIES}
	Boost::program_options
	JPEG::JPEG
)

# Workaround for a boost log ABI bug - https://github.com/microsoft/vcpkg/discussions/22762
target_compile_definitions(talyn PUBLIC BOOST_USE_WINAPI_VERSION=0x0A00)
target_compile_definitions(talyn PUBLIC _WIN32_WINNT=0x0A00)
target_compile_definitions(talyn PUBLIC WINVER=0x0A00)

target_compile_options(talyn PRIVATE /EHsc /utf-8)

# The driver resolves --file against the working directory, so the demo scene has to sit
# beside the executable to be reachable from the build tree.
v3d_add_app_data(talyn)
