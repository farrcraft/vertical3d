# The functions every target in the tree is built with. Separate from v3dDependencies.cmake
# so a consumer can include these without re-running the package resolution.
#
# Paths into this repository go through V3D_ROOT rather than CMAKE_SOURCE_DIR, which names
# the consumer's root once another project adds this one - see ADR-0027.

# Every library under api/ is declared with this, which gives it the four things a target
# has to carry to be linkable from outside this tree:
#
#  - an include root, so a consumer writes #include <api/image/Image.h>. The root is the
#    repository rather than api/, because api headers reach each other by relative path and
#    no other prefix leaves those resolving unchanged. There is no INSTALL_INTERFACE half:
#    ADR-0027 installs nothing, and a half-written export is worse than none.
#  - a v3d:: alias, which is the name a consumer links and the one that would survive a
#    later move to an installed package.
#  - /EHsc and /utf-8 in the interface. Both are carried by the directory's own flags for
#    this tree's compilation and reach nothing beyond it; a consumer that compiles Logger.h
#    without /utf-8 hits the static_assert in spdlog's bundled fmt.
#  - the boost log ABI workaround - https://github.com/microsoft/vcpkg/discussions/22762 -
#    which was on nine of the sixteen libraries and is a Windows API version selection, so
#    the ones it was missing from were the inconsistency it exists to prevent.
function(v3d_add_api_library name)
	set(target "v3dlib_${name}")
	add_library(${target} ${ARGN})
	add_library(v3d::${name} ALIAS ${target})
	target_include_directories(${target} PUBLIC $<BUILD_INTERFACE:${V3D_ROOT}>)
	target_compile_options(${target} INTERFACE /EHsc /utf-8)
	target_compile_definitions(${target} PUBLIC BOOST_USE_WINAPI_VERSION=0x0A00 _WIN32_WINNT=0x0A00 WINVER=0x0A00)
	# Every library names boost in a header, if only for shared_ptr.
	target_link_libraries(${target} PUBLIC Boost::headers)
endfunction()

# Assets shared by more than one app live in the root data/ directory, so there is one
# committed copy rather than one per app. v3d_add_shared_data copies them into an app's
# runtime data directory after it links, alongside whatever that app keeps in <app>/data.
# The engine resolves assets relative to the executable, so the destination has to be
# beside the exe rather than in the source tree.
function(v3d_add_shared_data target)
	add_custom_command(TARGET ${target} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
			"${V3D_ROOT}/data"
			"$<TARGET_FILE_DIR:${target}>/data"
		COMMENT "Copying shared data for ${target}"
		VERBATIM)
endfunction()

# An app's own assets are committed under <app>/data. They are copied beside the executable
# after it links for the same reason the shared ones are - the engine resolves every asset
# relative to the exe - and because a build tree that is not refreshed from the source tree
# drifts silently: editing <app>/data then appears to do nothing.
function(v3d_add_app_data target)
	add_custom_command(TARGET ${target} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
			"${CMAKE_CURRENT_SOURCE_DIR}/data"
			"$<TARGET_FILE_DIR:${target}>/data"
		COMMENT "Copying data for ${target}"
		VERBATIM)
endfunction()

# Tests live next to the library they cover, in api/<lib>/tests, and each directory builds
# one Boost.Test binary. v3d_add_test wires up what every one of them needs: the framework
# (whose imported target carries BOOST_TEST_DYN_LINK, since vcpkg builds boost shared),
# /EHsc and /utf-8 - the latter is required wherever spdlog's bundled fmt is compiled - and
# a working directory beside the executable, so a test's data files resolve the same way an
# app's do. Run them with ctest --test-dir out/build/<config>.
function(v3d_add_test lib)
	set(target "v3dtest_${lib}")
	add_executable(${target} ${ARGN})
	target_link_libraries(${target} PRIVATE Boost::unit_test_framework)
	target_compile_options(${target} PRIVATE /EHsc /utf-8)
	# Boost.Test's CRT leak check reports at exit, before spdlog's global registry is torn
	# down, so any test that builds a Logger reports the same permanent false positive.
	add_test(NAME ${lib} COMMAND ${target} --detect_memory_leaks=0 WORKING_DIRECTORY $<TARGET_FILE_DIR:${target}>)
endfunction()

# Shaders are compiled to SPIR-V at build time and embedded in the library that draws with
# them, rather than shipped as data files: they belong to the engine rather than to any app,
# so no app's data directory is the right place for them. glslc's -mfmt=c writes the module
# out as a C initialiser list, which the source includes into a uint32_t array.
#
# The tool is looked for here rather than at configure time, so that a build compiling no
# shader is not stopped by its absence.
function(v3d_add_shader target source)
	if(NOT Vulkan_GLSLC_EXECUTABLE)
		find_program(Vulkan_GLSLC_EXECUTABLE NAMES glslc HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin")
	endif()
	if(NOT Vulkan_GLSLC_EXECUTABLE)
		message(FATAL_ERROR "glslc was not found - it ships with the Vulkan SDK, which VULKAN_SDK should point at")
	endif()
	get_filename_component(name ${source} NAME)
	set(output "${CMAKE_CURRENT_BINARY_DIR}/shaders/${name}.inc")
	add_custom_command(
		OUTPUT ${output}
		COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders"
		COMMAND ${Vulkan_GLSLC_EXECUTABLE} --target-env=vulkan1.3 -O -mfmt=c "${CMAKE_CURRENT_SOURCE_DIR}/${source}" -o ${output}
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${source}"
		COMMENT "Compiling ${source} to SPIR-V"
		VERBATIM)
	set_source_files_properties(${output} PROPERTIES HEADER_FILE_ONLY TRUE GENERATED TRUE)
	target_sources(${target} PRIVATE ${output})
	target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
endfunction()
