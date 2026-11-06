add_executable(imagetool "SheetRequest.h" "imagetool.cxx")

# Boost::program_options is named here rather than reached through a library: imagetool
# parses its own command line.
target_link_libraries(imagetool PRIVATE
		v3dlib_log
		v3dlib_image
		v3dlib_asset
		v3dlib_asset_media
		v3dlib_version
		Boost::program_options
)

# draws an app's SVG icon into the .ico its executable embeds. v3d_add_app_icon runs it
add_executable(appicon "appicon.cxx")
target_link_libraries(appicon PRIVATE v3dlib_log v3dlib_image v3dlib_asset)

v3d_add_app_icon(imagetool "icon.svg")
v3d_install_app(imagetool "imagetool")

