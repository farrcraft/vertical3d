# The image and glTF loaders, registered on a manager the way engine::Engine does.
# None of them needs a window or a device.
v3d_add_test(asset_media
	"TestMain.cpp"
	"GltfTest.cpp"
	"MediaTest.cpp"
	"ThreadTest.cpp"
	"SheetPackingTest.cpp"
	"SpriteSheetsTest.cpp")

target_link_libraries(v3dtest_asset_media PRIVATE
	v3dlib_asset_media
	v3dlib_animation
	v3dlib_asset
	v3dlib_log
	v3dlib_image
	v3dlib_model
	${Boost_LIBRARIES}
	${PNG_LIBRARIES}
	JPEG::JPEG)

# The fixtures belong to the asset suite, and the render device suite reads them from there
# too. They are copied from that directory rather than kept twice. The scripts beside them
# generate most of the glTF ones. accessor_overrun.gltf, inverse_bind_overrun.gltf and
# inverse_bind_not_matrices.gltf are written by hand.
v3d_copy_directory(v3dtest_asset_media fixtures "${CMAKE_CURRENT_SOURCE_DIR}/../../tests/data" "data")
