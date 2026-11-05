# The install tree a release ships. On Windows it is one flat directory: every app, the DLLs they
# load, the MSVC runtime, data/ with each app's own assets in data/<executable>/, OpenUSD's
# plugins in usd/, and NOTICES.txt. The engine finds data/ beside the executable, and the editor
# finds moya beside itself, so an app runs from the install as it does from the build.
#
# Each app adds itself with v3d_install_app, and the root calls v3d_install_release once after
# every app has, then v3d_package_release. Other platforms have no install rules yet.

# The version a release shows, as the apps' version resources and the installer name it. It is
# set when the tree is configured, so a build that is not a release shows the number marked as a
# development build, without the commit the apps' own text names.
function(v3d_release_version out)
	if(NOT V3D_RELEASE)
		set(${out} "${vertical3d_VERSION}-dev" PARENT_SCOPE)
	elseif(V3D_PRERELEASE STREQUAL "")
		set(${out} "${vertical3d_VERSION}" PARENT_SCOPE)
	else()
		set(${out} "${vertical3d_VERSION}-${V3D_PRERELEASE}" PARENT_SCOPE)
	endif()
endfunction()

# Install an app's executable into the install root, and give it a version resource, which
# Windows shows in the file's properties. v3d_install_release installs the DLLs it loads, once for
# every app, and the installer gives each windowed app a Start menu entry.
#
# @param target the app's executable
# @param name what the app is called, in its Start menu entry and its version resource
function(v3d_install_app target name)
	if(NOT WIN32)
		return()
	endif()
	install(TARGETS ${target} RUNTIME DESTINATION .)
	set_property(GLOBAL APPEND PROPERTY V3D_INSTALLED_APPS "${target}")
	set_property(GLOBAL PROPERTY V3D_APP_NAME_${target} "${name}")

	v3d_release_version(version)
	set(numbers "${vertical3d_VERSION_MAJOR},${vertical3d_VERSION_MINOR},${vertical3d_VERSION_PATCH},0")
	set(script "${CMAKE_CURRENT_BINARY_DIR}/${target}_version.rc")
	file(GENERATE OUTPUT "${script}" CONTENT "1 VERSIONINFO
FILEVERSION ${numbers}
PRODUCTVERSION ${numbers}
FILEOS 0x40004
FILETYPE 0x1
BEGIN
    BLOCK \"StringFileInfo\"
    BEGIN
        BLOCK \"040904B0\"
        BEGIN
            VALUE \"CompanyName\", \"Vertical3D\"
            VALUE \"FileDescription\", \"${name}\"
            VALUE \"FileVersion\", \"${version}\"
            VALUE \"InternalName\", \"${target}\"
            VALUE \"LegalCopyright\", \"Copyright (c) Joshua Farr, under the MIT License\"
            VALUE \"OriginalFilename\", \"$<TARGET_FILE_NAME:${target}>\"
            VALUE \"ProductName\", \"Vertical3D\"
            VALUE \"ProductVersion\", \"${version}\"
        END
    END
    BLOCK \"VarFileInfo\"
    BEGIN
        VALUE \"Translation\", 0x409, 1200
    END
END
")
	target_sources(${target} PRIVATE "${script}")
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

# The Windows installer, built by CPack's NSIS generator from the install rules: cpack in the
# build directory writes it. It installs the tree into Program Files, adds a Start menu entry for
# each windowed app, and an uninstaller. It registers no file types. A macro, because CPack reads
# its variables from the scope that includes it.
macro(v3d_package_release)
	if(WIN32)
		v3d_release_version(v3d_package_version)
		set(v3d_package_entries)
		get_property(v3d_package_apps GLOBAL PROPERTY V3D_INSTALLED_APPS)
		foreach(v3d_package_app IN LISTS v3d_package_apps)
			get_target_property(v3d_package_windowed ${v3d_package_app} WIN32_EXECUTABLE)
			if(v3d_package_windowed)
				get_property(v3d_package_name GLOBAL PROPERTY V3D_APP_NAME_${v3d_package_app})
				list(APPEND v3d_package_entries "${v3d_package_app}" "${v3d_package_name}")
			endif()
		endforeach()

		set(CPACK_GENERATOR NSIS)
		set(CPACK_PACKAGE_NAME "Vertical3D")
		set(CPACK_PACKAGE_VENDOR "Vertical3D")
		set(CPACK_PACKAGE_VERSION "${v3d_package_version}")
		set(CPACK_PACKAGE_INSTALL_DIRECTORY "Vertical3D")
		set(CPACK_PACKAGE_FILE_NAME "vertical3d-${v3d_package_version}-windows-x64-setup")
		set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/package")
		set(CPACK_PACKAGE_EXECUTABLES ${v3d_package_entries})
		set(CPACK_RESOURCE_FILE_LICENSE "${V3D_ROOT}/LICENSE")
		set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
		set(CPACK_NSIS_INSTALLED_ICON_NAME "vertical3d.exe")
		set(CPACK_NSIS_DISPLAY_NAME "Vertical3D ${v3d_package_version}")
		set(CPACK_NSIS_PACKAGE_NAME "Vertical3D")
		set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
		include(CPack)
	endif()
endmacro()
