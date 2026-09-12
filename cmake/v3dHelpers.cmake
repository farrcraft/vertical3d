# The functions every target in the tree is built with. Separate from v3dDependencies.cmake
# so a consumer can include these without re-running the package resolution.
#
# Paths into this repository go through V3D_ROOT rather than CMAKE_SOURCE_DIR, which names
# the consumer's root once another project adds this one - see ADR-0027.

# Every library under api/ is declared with this, which gives it the three things a target
# has to carry to be linkable from outside this tree:
#
#  - an include root, so every file writes #include <api/image/Image.h> - a consumer's and
#    this repository's alike, per ADR-0048. The root is the repository rather than api/ so
#    that the prefix says which repository a header came from, and because sixteen per-library
#    roots would put names like <type/Camera.h> on every consumer's search path. There is no
#    INSTALL_INTERFACE half: ADR-0027 installs nothing, and a half-written export is worse
#    than none.
#  - a v3d:: alias, which is the name a consumer links and the one that would survive a
#    later move to an installed package.
#  - /EHsc and /utf-8 in the interface. Both are carried by the directory's own flags for
#    this tree's compilation and reach nothing beyond it; a consumer that compiles Logger.h
#    without /utf-8 hits the static_assert in spdlog's bundled fmt.
function(v3d_add_api_library name)
	set(target "v3dlib_${name}")
	add_library(${target} ${ARGN})
	add_library(v3d::${name} ALIAS ${target})
	target_include_directories(${target} PUBLIC $<BUILD_INTERFACE:${V3D_ROOT}>)
	target_compile_options(${target} INTERFACE /EHsc /utf-8)
	# Every library names boost in a header, if only for shared_ptr.
	target_link_libraries(${target} PUBLIC Boost::headers)
endfunction()

# Copy a directory of assets beside a target's executable, as a build rule that the assets
# themselves are the inputs to.
#
# **Not a POST_BUILD step on the target**, which is what this was until it was found to do
# the opposite of what its own comment claimed. POST_BUILD runs only when the target itself
# relinks, so editing a document and rebuilding left the previous copy in place and the app
# went on reading it - the silent drift between source tree and build tree that copying is
# supposed to prevent. Ninja reports "no work to do" while the running app disagrees with
# the file on disk, and the wrong conclusion to draw from that is that the edit had no
# effect.
#
# A stamp file is the rule's output because a directory is not a dependency a generator can
# compare timestamps on. CONFIGURE_DEPENDS re-globs when the build runs rather than only at
# configure time, so a document that is added rather than edited is picked up as well - the
# case that is easiest to miss, because it looks exactly like the edit case from outside.
#
# The stamp is per configuration. A multi-config generator gives each configuration its own
# TARGET_FILE_DIR, and one shared stamp would leave the second configuration's data
# directory unwritten.
#
# **A deleted asset is still left behind**, because copy_directory merges rather than
# mirrors. Clearing the destination first is not available here: an app takes the shared
# data and its own into the same directory, so whichever copy ran second would clear what
# the first had just written, and re-running only the copy whose own glob changed would not
# put the other back. The failure is a file nothing reads rather than an app reading the
# wrong one, which is the difference between this and the case above - a stale data
# directory is cleared by deleting it, and the next build fills it.
#
# @param target the executable the data is copied beside
# @param name what this set of data is, which separates one call's stamp from another's
# @param source the directory to copy
function(v3d_copy_data target name source)
	file(GLOB_RECURSE files CONFIGURE_DEPENDS "${source}/*")
	set(copier "${target}_${name}_data")
	set(stamp "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${copier}.stamp")
	add_custom_command(OUTPUT "${stamp}"
		COMMAND ${CMAKE_COMMAND} -E copy_directory "${source}" "$<TARGET_FILE_DIR:${target}>/data"
		COMMAND ${CMAKE_COMMAND} -E touch "${stamp}"
		DEPENDS ${files}
		COMMENT "Copying ${name} data for ${target}"
		VERBATIM)
	add_custom_target(${copier} DEPENDS "${stamp}")
	add_dependencies(${target} ${copier})
	# An app that takes both the shared data and its own copies two directories into one
	# destination, and nothing orders those against each other - so they are chained in the
	# order they were asked for rather than left to run at the same time.
	get_target_property(previous ${target} V3D_LAST_DATA_TARGET)
	if(previous)
		add_dependencies(${copier} ${previous})
	endif()
	set_target_properties(${target} PROPERTIES V3D_LAST_DATA_TARGET ${copier})
endfunction()

# Assets shared by more than one app live in the root data/ directory, so there is one
# committed copy rather than one per app. v3d_add_shared_data copies them into an app's
# runtime data directory, alongside whatever that app keeps in <app>/data. The engine
# resolves assets relative to the executable, so the destination has to be beside the exe
# rather than in the source tree.
function(v3d_add_shared_data target)
	v3d_copy_data(${target} shared "${V3D_ROOT}/data")
endfunction()

# An app's own assets are committed under <app>/data, and are copied beside the executable
# for the same reason the shared ones are.
function(v3d_add_app_data target)
	v3d_copy_data(${target} app "${CMAKE_CURRENT_SOURCE_DIR}/data")
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
	# A suite names its subject from the repository root, per ADR-0048. Most get the root
	# from the api library they cover, whose include directory is PUBLIC; a suite that links
	# no api library - voxel's meshing tests link only boost and libnoise - has no other
	# source for it, and every one of its includes fails to resolve without this.
	target_include_directories(${target} PRIVATE ${V3D_ROOT})
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
