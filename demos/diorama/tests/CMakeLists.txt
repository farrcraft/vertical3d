# The village's layout and its sun need neither a window nor a device, and every model it places
# is decoded here as the renderer would read it, with no upload. The engine is built so its
# commands can be checked against the bindings and the menu the diorama ships, and is never
# started. The renderer is not covered.
v3d_add_test(diorama
	"TestMain.cxx"
	"BindingsTest.cxx"
	"LayoutTest.cxx"
	"../src/Diorama.cxx"
	"../src/Layout.cxx"
	"../src/Renderer.cxx")

target_link_libraries(v3dtest_diorama PRIVATE
	v3dlib_asset
	v3dlib_asset_media
	v3dlib_audio
	v3dlib_ecs
	v3dlib_engine
	v3dlib_input
	v3dlib_log
	v3dlib_render
	v3dlib_shell
	v3dlib_type
	v3dlib_ui
	v3dlib_window)

v3d_copy_data(v3dtest_diorama app "${CMAKE_CURRENT_SOURCE_DIR}/../data")
