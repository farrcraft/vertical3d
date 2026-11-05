# Install the DLLs the installed apps load. Run by the install, after every app is in place.
#
# The DLLs are resolved from the installed executables rather than from the build's. In the
# build, vcpkg copies each DLL beside every app that loads it, and those copies are the same file
# in many places. Beside the installed apps there is no DLL yet, so each resolves to vcpkg's own.
#
# @param V3D_DLL_DIRECTORY vcpkg's directory of DLLs for the build's configuration
# @param CMAKE_INSTALL_PREFIX the install root, which holds the apps

file(GLOB apps "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/*.exe")
if(NOT apps)
	message(FATAL_ERROR "no app is installed in ${CMAKE_INSTALL_PREFIX} to find the DLLs of")
endif()

# The system's DLLs are left out, and so is the Vulkan loader, which comes with the GPU driver
file(GET_RUNTIME_DEPENDENCIES
	EXECUTABLES ${apps}
	DIRECTORIES "${V3D_DLL_DIRECTORY}"
	PRE_EXCLUDE_REGEXES "^api-ms-" "^ext-ms-" "^vulkan-1\\.dll$"
	POST_EXCLUDE_REGEXES "[/\\\\][Ss]ystem32[/\\\\]"
	RESOLVED_DEPENDENCIES_VAR resolved
	UNRESOLVED_DEPENDENCIES_VAR unresolved)
if(unresolved)
	message(FATAL_ERROR "the installed apps load DLLs that are not in ${V3D_DLL_DIRECTORY}: ${unresolved}")
endif()
file(INSTALL ${resolved} DESTINATION "${CMAKE_INSTALL_PREFIX}")
