# What each api library depends on, and the only place in the tree that says so before any
# of them has been configured.
#
# A consumer names the libraries it links in V3D_LIBRARIES and gets those and their closure -
# nothing else is added and no package outside that closure is looked for, so an app wanting
# only v3d::image does not need the Vulkan SDK to configure. See ADR-0033.
#
# This is a second statement of what each library's own target_link_libraries already says,
# which is a fact recorded twice. v3d_api_verify_manifest below is what stops the two
# disagreeing: it reads the real link graph once every library has been added and fails the
# configure on a difference in either direction.

# Every api library, by the name that follows v3dlib_ and v3d::.
set(V3D_API_LIBRARIES
	asset audio brep config dag ecs engine event font grid image input log
	render render_offline type ui)

# PATH is the directory under api/ holding the library, which is the name except where a
# library sits under another one's directory. REQUIRES is the api libraries it links;
# PACKAGES is the third party packages it names.
#
# Boost is in no PACKAGES list: v3d_add_api_library links Boost::headers into every library,
# so there is no selection that does not need it and v3dDependencies.cmake resolves it
# unconditionally.

set(V3D_API_asset_PATH "asset")
set(V3D_API_asset_REQUIRES log audio font image type)
set(V3D_API_asset_PACKAGES cgltf)

set(V3D_API_audio_PATH "audio")
set(V3D_API_audio_REQUIRES log asset event)
set(V3D_API_audio_PACKAGES SDL3_mixer EnTT)

set(V3D_API_brep_PATH "brep")
set(V3D_API_brep_REQUIRES dag type)
set(V3D_API_brep_PACKAGES glm)

set(V3D_API_config_PATH "config")
set(V3D_API_config_REQUIRES log asset type)
set(V3D_API_config_PACKAGES glm)

set(V3D_API_dag_PATH "dag")
set(V3D_API_dag_REQUIRES)
set(V3D_API_dag_PACKAGES glm)

set(V3D_API_ecs_PATH "ecs")
set(V3D_API_ecs_REQUIRES)
set(V3D_API_ecs_PACKAGES glm EnTT)

set(V3D_API_engine_PATH "engine")
set(V3D_API_engine_REQUIRES log asset config event input render)
set(V3D_API_engine_PACKAGES SDL3 EnTT)

set(V3D_API_event_PATH "event")
set(V3D_API_event_REQUIRES)
set(V3D_API_event_PACKAGES glm EnTT)

set(V3D_API_font_PATH "font")
set(V3D_API_font_REQUIRES log image type)
set(V3D_API_font_PACKAGES Freetype glm)

set(V3D_API_grid_PATH "grid")
set(V3D_API_grid_REQUIRES)
set(V3D_API_grid_PACKAGES glm)

set(V3D_API_image_PATH "image")
set(V3D_API_image_REQUIRES log)
set(V3D_API_image_PACKAGES PNG JPEG glm)

set(V3D_API_input_PATH "input")
set(V3D_API_input_REQUIRES event)
set(V3D_API_input_PACKAGES SDL3 glm EnTT)

set(V3D_API_log_PATH "log")
set(V3D_API_log_REQUIRES)
set(V3D_API_log_PACKAGES spdlog)

set(V3D_API_render_PATH "render")
set(V3D_API_render_REQUIRES log asset font image)
set(V3D_API_render_PACKAGES Vulkan VulkanMemoryAllocator SDL3 glm EnTT)

# Under api/render but not part of it: the offline renderers of ADR-0022 share this and it
# names neither Vulkan nor SDL, which is the whole reason it is selectable on its own.
set(V3D_API_render_offline_PATH "render/offline")
set(V3D_API_render_offline_REQUIRES log image type)
set(V3D_API_render_offline_PACKAGES glm)

set(V3D_API_type_PATH "type")
set(V3D_API_type_REQUIRES)
set(V3D_API_type_PACKAGES glm)

set(V3D_API_ui_PATH "ui")
set(V3D_API_ui_REQUIRES log render asset event font image type)
set(V3D_API_ui_PACKAGES glm EnTT)

# The imported target each package provides, which is how the verification below recognises
# a package in a link line. A package whose whole contribution is an include directory -
# cgltf is the only one - has no target and so cannot be checked this way.
set(V3D_PACKAGE_TARGETS
	"Vulkan::Vulkan=Vulkan"
	"GPUOpen::VulkanMemoryAllocator=VulkanMemoryAllocator"
	"SDL3::SDL3=SDL3"
	"SDL3_mixer::SDL3_mixer=SDL3_mixer"
	"Freetype::Freetype=Freetype"
	"PNG::PNG=PNG"
	"JPEG::JPEG=JPEG"
	"glm::glm=glm"
	"EnTT::EnTT=EnTT"
	"spdlog::spdlog=spdlog")

# A package whose whole contribution is an include directory has no target to appear in a
# link line, so nothing can confirm the library still uses it and the verification below has
# to take the manifest's word for it.
set(V3D_PACKAGES_WITHOUT_TARGET cgltf)

# Expand V3D_LIBRARIES into the set of libraries to add and the set of packages to look for.
# Sets V3D_API_BUILD and V3D_API_PACKAGES in the caller's scope.
function(v3d_api_closure)
	set(pending ${V3D_LIBRARIES})
	set(closure)
	while(pending)
		list(POP_FRONT pending library)
		if(library IN_LIST closure)
			continue()
		endif()
		if(NOT library IN_LIST V3D_API_LIBRARIES)
			list(JOIN V3D_API_LIBRARIES " " known)
			message(FATAL_ERROR
				"V3D_LIBRARIES names '${library}', which is not an api library. "
				"The set is: ${known}")
		endif()
		list(APPEND closure ${library})
		list(APPEND pending ${V3D_API_${library}_REQUIRES})
	endwhile()

	set(packages)
	foreach(library IN LISTS closure)
		list(APPEND packages ${V3D_API_${library}_PACKAGES})
	endforeach()
	list(REMOVE_DUPLICATES packages)

	# alphabetical rather than in the order the closure was walked, so the configure output
	# and any diff of it are stable
	list(SORT closure)
	list(SORT packages)
	set(V3D_API_BUILD "${closure}" PARENT_SCOPE)
	set(V3D_API_PACKAGES "${packages}" PARENT_SCOPE)
endfunction()

# One manifest field of one library against what the link lines actually named. Appends to
# the list named by out_var, which is why it takes the name rather than the value.
function(v3d_api_compare library field actual out_var)
	set(declared ${V3D_API_${library}_${field}})
	set(problems ${${out_var}})
	foreach(entry IN LISTS actual)
		if(NOT entry IN_LIST declared)
			list(APPEND problems
				"  v3dlib_${library} links ${entry}, which V3D_API_${library}_${field} does not name")
		endif()
	endforeach()
	foreach(entry IN LISTS declared)
		if(entry IN_LIST V3D_PACKAGES_WITHOUT_TARGET)
			continue()
		endif()
		if(NOT entry IN_LIST actual)
			list(APPEND problems
				"  V3D_API_${library}_${field} names ${entry}, which v3dlib_${library} does not link")
		endif()
	endforeach()
	set(${out_var} "${problems}" PARENT_SCOPE)
endfunction()

# Read the link graph the libraries actually declared and compare it to the manifest above.
# Called once every api library has been added.
#
# A library missing from a REQUIRES list is the failure that matters: this tree always builds
# the whole api, so nothing here would notice until a consumer selected a narrow set and got
# an unknown target. The reverse - a manifest entry no link line backs - is reported too,
# because a stale entry drags a package into a configure that no longer needs it.
function(v3d_api_verify_manifest)
	set(problems)
	foreach(library IN LISTS V3D_API_BUILD)
		set(target "v3dlib_${library}")
		get_target_property(linked ${target} LINK_LIBRARIES)
		get_target_property(interface ${target} INTERFACE_LINK_LIBRARIES)
		set(named)
		foreach(list_ IN ITEMS linked interface)
			if(${list_})
				list(APPEND named ${${list_}})
			endif()
		endforeach()
		list(REMOVE_DUPLICATES named)

		# what the link lines say, in the manifest's vocabulary
		set(requires_)
		set(packages)
		foreach(entry IN LISTS named)
			if(entry MATCHES "^v3dlib_(.+)$")
				list(APPEND requires_ "${CMAKE_MATCH_1}")
			elseif(entry MATCHES "^Boost::")
				# resolved unconditionally, so no manifest entry claims it
			else()
				foreach(mapping IN LISTS V3D_PACKAGE_TARGETS)
					string(REPLACE "=" ";" mapping "${mapping}")
					list(GET mapping 0 imported)
					list(GET mapping 1 package)
					if(entry STREQUAL imported)
						list(APPEND packages "${package}")
					endif()
				endforeach()
			endif()
		endforeach()
		list(REMOVE_DUPLICATES requires_)
		list(REMOVE_DUPLICATES packages)

		v3d_api_compare("${library}" REQUIRES "${requires_}" problems)
		v3d_api_compare("${library}" PACKAGES "${packages}" problems)
	endforeach()

	if(problems)
		list(JOIN problems "\n" problems)
		message(FATAL_ERROR
			"The api manifest in cmake/v3dApiLibraries.cmake disagrees with the link "
			"graph:\n${problems}\n"
			"Update the manifest to match.")
	endif()
endfunction()
