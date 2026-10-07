# The image and glTF loaders, registered on a manager the way engine::Engine does.
# None of them needs a window or a device.
v3d_add_test(asset_media
	"TestMain.cpp"
	"GltfTest.cpp"
	"MediaTest.cpp")

target_link_libraries(v3dtest_asset_media PRIVATE
	v3dlib_asset_media
	v3dlib_asset
	v3dlib_log
	v3dlib_image
	v3dlib_type
	${Boost_LIBRARIES}
	${PNG_LIBRARIES}
	JPEG::JPEG)

# The fixtures belong to the asset suite. The scripts beside them generate the glTF ones,
# and the render device suite reads them from there too. They are copied from that directory
# rather than kept twice.
add_custom_command(TARGET v3dtest_asset_media POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/../../tests/data"
		"$<TARGET_FILE_DIR:v3dtest_asset_media>/data"
	COMMENT "Copying asset media test fixtures"
	VERBATIM)
