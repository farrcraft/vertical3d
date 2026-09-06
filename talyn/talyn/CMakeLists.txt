add_executable("talyn" "talyn.cxx")

target_link_libraries("talyn" PRIVATE
	v3dlib_talyn
	v3dlib_image
	Boost::program_options
)

target_compile_options(talyn PRIVATE /EHsc /utf-8)

# The driver resolves --file against the working directory, so the demo scene has to sit
# beside the executable to be reachable from the build tree.
v3d_add_app_data(talyn)
