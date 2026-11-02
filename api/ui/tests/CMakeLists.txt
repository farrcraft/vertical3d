# The ui is testable without a window or a device: ComponentRenderer draws onto a
# realtime::canvas::Canvas, which is cpu side, and takes text measuring and writing as callbacks.
v3d_add_test(ui
	"TestMain.cpp"
	"ArrangerTest.cpp"
	"CheckBoxTest.cpp"
	"ComponentRendererTest.cpp"
	"CurveTextTest.cpp"
	"CursorTest.cpp"
	"DocumentTest.cpp"
	"FileChooserTest.cpp"
	"FocusTest.cpp"
	"GameMenuTest.cpp"
	"GlyphPagesTest.cpp"
	"ImageTest.cpp"
	"ImmediateInputTest.cpp"
	"ImmediatePanelTest.cpp"
	"ImmediatePopupTest.cpp"
	"ImmediateTableTest.cpp"
	"ImmediateTest.cpp"
	"ImmediateTextFieldTest.cpp"
	"ImmediateWindowTest.cpp"
	"KeyboardDriveTest.cpp"
	"LayoutTest.cpp"
	"LineEditTest.cpp"
	"MenuBarTest.cpp"
	"PainterTest.cpp"
	"PaletteTest.cpp"
	"PromptTest.cpp"
	"ResolverTest.cpp"
	"ScreenTest.cpp"
	"ScrollbarTest.cpp"
	"ShellKeyboardTest.cpp"
	"SliderTest.cpp"
	"SelectListTest.cpp"
	"StatisticsOverlayTest.cpp"
	"TabBarTest.cpp"
	"TextBoxTest.cpp"
	"TextRendererTest.cpp"
	"ThemeTest.cpp"
	"ToolbarTest.cpp"
	"TooltipTest.cpp"
	"TransitionTest.cpp"
	"TypeTest.cpp")

target_link_libraries(v3dtest_ui PRIVATE
	v3dlib_ui
	v3dlib_font
	v3dlib_render
	v3dlib_log
	v3dlib_event
	v3dlib_asset
	v3dlib_type
	EnTT::EnTT
	${Boost_LIBRARIES}
	${SDL3_LIBRARIES}
	${Vulkan_LIBRARIES}
	Freetype::Freetype)

# TextRendererTest packs real glyphs, so the shared font has to sit beside the executable.
v3d_add_shared_data(v3dtest_ui)

# The faces in fonts/ are fixtures: a monospaced face the cases measure predictably, and a face
# that sets Hebrew. The editor's suite copies them too. The font library's colour fixtures beside
# them draw an emoji's layers, its gradients, and its picture.
add_custom_command(TARGET v3dtest_ui POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/fonts"
		"$<TARGET_FILE_DIR:v3dtest_ui>/fonts"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${V3D_ROOT}/api/font/tests/fonts/colr0.ttf"
		"${V3D_ROOT}/api/font/tests/fonts/colr1.ttf"
		"${V3D_ROOT}/api/font/tests/fonts/cbdt.ttf"
		"$<TARGET_FILE_DIR:v3dtest_ui>/fonts"
	COMMENT "Copying ui test fonts"
	VERBATIM)
