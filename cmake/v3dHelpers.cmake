# The functions every target in the tree is built with. Separate from v3dDependencies.cmake
# so a consumer can include these without re-running the package resolution.
#
# Paths into this repository go through V3D_ROOT rather than CMAKE_SOURCE_DIR, which names
# the consumer's root once another project adds this one with add_subdirectory.

# Every library under api/ is declared with this, which gives it the three things a target
# has to carry to be linkable from outside this tree:
#
#  - an include root, so every file, in a consumer or in this repository, names a header
#    from the repository root, as in #include <api/image/Image.h>. The root is the
#    repository, not api/. The prefix then shows which repository a header came from, and
#    names like <type/Camera.h> stay off every consumer's search path.
#    There is no INSTALL_INTERFACE half, because the api is consumed as source and nothing
#    is installed.
#  - a v3d:: alias, which is the name a consumer links. It would stay the same if the api
#    were later shipped as an installed package.
#  - /EHsc and /utf-8 under MSVC, for the library and for whatever compiles its headers. A
#    consumer that compiles Logger.h without /utf-8 hits the static_assert in spdlog's
#    bundled fmt. Clang and GCC need neither: they enable exceptions, and read and write
#    UTF-8, by default.
function(v3d_add_api_library name)
	set(target "v3dlib_${name}")
	add_library(${target} ${ARGN})
	add_library(v3d::${name} ALIAS ${target})
	target_include_directories(${target} PUBLIC $<BUILD_INTERFACE:${V3D_ROOT}>)
	if(MSVC)
		target_compile_options(${target} PUBLIC /EHsc /utf-8)
	endif()
	# Most libraries name boost in a header, if only for shared_ptr, so every one is given
	# the headers.
	target_link_libraries(${target} PUBLIC Boost::headers)
endfunction()

# Copy a directory of assets beside a target's executable, as a build rule that the assets
# themselves are the inputs to.
#
# **Not a POST_BUILD step on the target.** POST_BUILD runs only when the target itself
# relinks, so an edited asset would not be copied until the next relink. Until then the app
# would keep reading the old copy while Ninja reports "no work to do".
#
# A stamp file is the rule's output because a generator cannot compare timestamps on a
# directory. CONFIGURE_DEPENDS re-globs when the build runs rather than only at configure
# time, so an asset that is added, not just edited, is copied as well.
#
# The stamp is per configuration. A multi-config generator gives each configuration its own
# TARGET_FILE_DIR, and one shared stamp would leave the second configuration's data
# directory unwritten.
#
# **A deleted asset is still left behind**, because copy_directory merges rather than
# mirrors. The destination cannot be cleared first: an app copies the shared data and its
# own into the same directory, so the second copy would clear what the first had written.
# A leftover file is harmless because nothing reads it. To clear a stale data directory,
# delete it and the next build fills it again.
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
	# destination, and nothing orders those copies against each other. They are chained in
	# the order they were requested rather than left to run at the same time.
	get_target_property(previous ${target} V3D_LAST_DATA_TARGET)
	if(previous)
		add_dependencies(${copier} ${previous})
	endif()
	set_target_properties(${target} PROPERTIES V3D_LAST_DATA_TARGET ${copier})
endfunction()

# Copy OpenUSD's plugins beside an executable that reads USD.
#
# OpenUSD reads usd/plugInfo.json beside its libraries, which names each plugin's directory
# under it, and stops at a breakpoint when it finds none. vcpkg copies the libraries beside each
# executable but not the plugins. It also installs the plugins beside the libraries, in bin/usd,
# and that one file in lib/usd, so both are copied. The sources are found from the imported usd
# target, so a Debug build copies the debug plugins.
#
# @param target the executable that links v3dlib_interchange_usd, directly or not
function(v3d_copy_usd_plugins target)
	add_custom_command(TARGET ${target} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_FILE_DIR:usd>/usd" "$<TARGET_FILE_DIR:${target}>/usd"
		COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE_DIR:usd>/../lib/usd/plugInfo.json" "$<TARGET_FILE_DIR:${target}>/usd/plugInfo.json"
		COMMENT "Copying OpenUSD's plugins for ${target}"
		VERBATIM)
endfunction()

# Copy another target's executable beside a target's own, for a target that runs it, with the
# shared libraries the program loads, which the target itself may not. The copy is a rule whose
# input is the executable, for the same reason v3d_copy_data's is. A POST_BUILD step would copy
# only when the target relinks, and leave an older build of the program beside it.
#
# @param target the executable that runs the program
# @param program the executable target it runs
function(v3d_copy_program target program)
	set(copier "${target}_${program}_program")
	set(stamp "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${copier}.stamp")
	add_custom_command(OUTPUT "${stamp}"
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${program}>" $<TARGET_RUNTIME_DLLS:${program}>
			"$<TARGET_FILE_DIR:${target}>"
		COMMAND ${CMAKE_COMMAND} -E touch "${stamp}"
		DEPENDS "$<TARGET_FILE:${program}>"
		COMMENT "Copying ${program} beside ${target}"
		COMMAND_EXPAND_LISTS
		VERBATIM)
	add_custom_target(${copier} DEPENDS "${stamp}")
	add_dependencies(${copier} ${program})
	add_dependencies(${target} ${copier})
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
# one Boost.Test binary. v3d_add_test wires up what every one of them needs: the framework,
# /EHsc and /utf-8 under MSVC, and a working directory beside the executable. The framework's imported
# target carries BOOST_TEST_DYN_LINK, since vcpkg builds boost shared. /utf-8 is required
# wherever spdlog's bundled fmt is compiled. The working directory lets a test's data files
# resolve the same way an app's do. Run them with ctest --test-dir out/build/<config>.
function(v3d_add_test lib)
	set(target "v3dtest_${lib}")
	add_executable(${target} ${ARGN})
	target_link_libraries(${target} PRIVATE Boost::unit_test_framework)
	# A suite includes headers by their path from the repository root. An api library gives
	# the root PUBLIC. An app suite also compiles the app's own sources, which include each
	# other by that path. The root is therefore named here, not left to a link.
	target_include_directories(${target} PRIVATE ${V3D_ROOT})
	if(MSVC)
		target_compile_options(${target} PRIVATE /EHsc /utf-8)
	endif()
	# Boost.Test's CRT leak check reports at exit, before spdlog's global registry is torn
	# down, so any test that builds a Logger reports the same permanent false positive.
	add_test(NAME ${lib} COMMAND ${target} --detect_memory_leaks=0 WORKING_DIRECTORY $<TARGET_FILE_DIR:${target}>)
endfunction()

# Shaders are compiled to SPIR-V at build time and embedded in the library that draws with
# them. They are not shipped as data files. They belong to the engine, not to any app, so no
# app's data directory is the right place for them. glslc's -mfmt=c writes the module
# out as a C initialiser list, which the source includes into a uint32_t array.
#
# The tool is looked for here rather than at configure time, so that a build compiling no
# shader is not stopped by its absence.
#
# OUTPUT names the module something other than the source's file name, and DEFINES are passed
# to the preprocessor, so that one source compiled twice is two shaders rather than two copies
# of a source.
function(v3d_add_shader target source)
	cmake_parse_arguments(PARSE_ARGV 2 shader "" "OUTPUT" "DEFINES")
	if(NOT Vulkan_GLSLC_EXECUTABLE)
		find_program(Vulkan_GLSLC_EXECUTABLE NAMES glslc HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin")
	endif()
	if(NOT Vulkan_GLSLC_EXECUTABLE)
		message(FATAL_ERROR "glslc was not found - it ships with the Vulkan SDK, which VULKAN_SDK should point at")
	endif()
	get_filename_component(name ${source} NAME)
	if(shader_OUTPUT)
		set(name ${shader_OUTPUT})
	endif()
	set(output "${CMAKE_CURRENT_BINARY_DIR}/shaders/${name}.inc")
	set(defines "")
	foreach(define IN LISTS shader_DEFINES)
		list(APPEND defines "-D${define}")
	endforeach()
	# glslc writes what the shader #included into a depfile, so an edit to a block several
	# shaders share rebuilds every one of them rather than only the file that was named here
	add_custom_command(
		OUTPUT ${output}
		COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders"
		COMMAND ${Vulkan_GLSLC_EXECUTABLE} --target-env=vulkan1.3 -O -mfmt=c ${defines} -MD -MF "${output}.d" -MT ${output}
			"${CMAKE_CURRENT_SOURCE_DIR}/${source}" -o ${output}
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${source}"
		DEPFILE "${output}.d"
		COMMENT "Compiling ${source} to SPIR-V as ${name}"
		VERBATIM)
	set_source_files_properties(${output} PROPERTIES HEADER_FILE_ONLY TRUE GENERATED TRUE)
	target_sources(${target} PRIVATE ${output})
	target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
endfunction()
