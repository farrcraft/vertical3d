# The install tree a release ships. On Windows it is one flat directory: every app, the DLLs they
# load, the MSVC runtime, data/ with each app's own assets in data/<executable>/, OpenUSD's
# plugins in usd/, and NOTICES.txt. The engine finds data/ beside the executable, and the editor
# finds moya beside itself, so an app runs from the install as it does from the build.
#
# Each app adds itself with v3d_install_app, and the root calls v3d_install_release once after
# every app has. Other platforms have no install rules yet.

# Install an app's executable into the install root. v3d_install_release installs the DLLs it
# loads, once for every app.
#
# @param target the app's executable
function(v3d_install_app target)
	if(WIN32)
		install(TARGETS ${target} RUNTIME DESTINATION .)
	endif()
endfunction()

# Install what every app shares: the shared data, the DLLs the apps load, the MSVC runtime and
# the notices.
function(v3d_install_release)
	if(NOT WIN32)
		return()
	endif()
	install(DIRECTORY "${V3D_ROOT}/data/" DESTINATION data)

	# The DLLs come from vcpkg's install, found from the installed apps. The root's rules run
	# after every subdirectory's, so the apps are in place by then.
	set(installed "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
	install(CODE "set(V3D_DLL_DIRECTORY \"$<IF:$<CONFIG:Debug>,${installed}/debug/bin,${installed}/bin>\")")
	install(SCRIPT "${V3D_ROOT}/cmake/v3dInstallDependencies.cmake")

	# The MSVC runtime, beside the apps, so a machine without the redistributable runs them
	set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION .)
	include(InstallRequiredSystemLibraries)

	# The notices are written from the install's packages as the tree is installed
	find_program(V3D_NODE node)
	if(NOT V3D_NODE)
		install(CODE "message(FATAL_ERROR \"installing writes NOTICES.txt with node, which is not on the PATH\")")
		return()
	endif()
	install(CODE "
		execute_process(COMMAND \"${V3D_NODE}\" \"${V3D_ROOT}/scripts/notices.ts\"
			--installed \"${VCPKG_INSTALLED_DIR}\" --out \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/NOTICES.txt\"
			RESULT_VARIABLE failed)
		if(failed)
			message(FATAL_ERROR \"scripts/notices.ts found something without a licence, and wrote no NOTICES.txt\")
		endif()")
endfunction()

# Install OpenUSD's plugins beside its DLLs, where OpenUSD looks for them, as
# v3d_copy_usd_plugins copies them into the build.
function(v3d_install_usd_plugins)
	if(WIN32)
		install(DIRECTORY "$<TARGET_FILE_DIR:usd>/usd" DESTINATION .)
		install(FILES "$<TARGET_FILE_DIR:usd>/../lib/usd/plugInfo.json" DESTINATION usd)
	endif()
endfunction()
