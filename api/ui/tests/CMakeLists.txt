# The ui is testable without a window or a device: ComponentRenderer draws onto a
# realtime::Canvas, which is cpu side, and takes text measuring and writing as callbacks.
v3d_add_test(ui
	"TestMain.cpp"
	"ArrangerTest.cpp"
	"CheckBoxTest.cpp"
	"ComponentRendererTest.cpp"
	"CursorTest.cpp"
	"GameMenuTest.cpp"
	"ImmediateTest.cpp"
	"LayoutTest.cpp"
	"MenuBarTest.cpp"
	"PainterTest.cpp"
	"ResolverTest.cpp"
	"ScrollbarTest.cpp"
	"SelectListTest.cpp"
	"StatisticsOverlayTest.cpp"
	"TabBarTest.cpp"
	"ThemeTest.cpp"
	"ToolbarTest.cpp")

target_link_libraries(v3dtest_ui PRIVATE
	v3dlib_ui
	v3dlib_render
	v3dlib_font
	v3dlib_event
	v3dlib_asset
	v3dlib_type
	EnTT::EnTT
	${Boost_LIBRARIES}
	${SDL3_LIBRARIES}
	${Vulkan_LIBRARIES}
	Freetype::Freetype)
