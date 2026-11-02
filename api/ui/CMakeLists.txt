v3d_add_api_library(ui
	"Arranger.h" "Arranger.cpp"
	"Component.h" "Component.cpp"
	"Container.h" "Container.cpp"
	"DrawOrder.h"
	"Interface.h" "Interface.cpp"
	"Image.h" "Image.cpp"
	"Immediate.h" "Immediate.cpp"
	"ImmediatePopups.cpp"
	"ImmediateTables.cpp"
	"ImmediateTextField.cpp"
	"ImmediateWindows.cpp"
	"Layout.h" "Layout.cpp"
	"Length.h" "Length.cpp"
	"Loader.h" "Loader.cpp"
	"Transition.h" "Transition.cpp"
	"component/Bar.h" "component/Bar.cpp"
	"component/Box.h" "component/Box.cpp"
	"component/Button.h" "component/Button.cpp"
	"component/CheckBox.h" "component/CheckBox.cpp"
	"component/HorizontalBox.h" "component/HorizontalBox.cpp"
	"component/Icon.h" "component/Icon.cpp"
	"component/Label.h" "component/Label.cpp"
	"component/Panel.h" "component/Panel.cpp"
	"component/RadioButton.h" "component/RadioButton.cpp"
	"component/Scrollbar.h" "component/Scrollbar.cpp"
	"component/Slider.h" "component/Slider.cpp"
	"component/SelectList.h" "component/SelectList.cpp"
	"component/TabBar.h" "component/TabBar.cpp"
	"component/TabPage.h" "component/TabPage.cpp"
	"component/TextBox.h" "component/TextBox.cpp"
	"component/Toolbar.h" "component/Toolbar.cpp"
	"component/Type.h" "component/Type.cpp"
	"component/VerticalBox.h" "component/VerticalBox.cpp"
	"component/menu/Menu.h" "component/menu/Menu.cpp"
	"component/menu/Bar.h" "component/menu/Bar.cpp"
	"component/menu/Item.h" "component/menu/Item.cpp"
	"component/menu/Type.h"
	"immediate/Context.h" "immediate/Context.cpp"
	"input/Command.h" "input/Command.cpp"
	"input/Cursor.h" "input/Cursor.cpp"
	"input/Keys.h" "input/Keys.cpp"
	"input/LineEdit.h" "input/LineEdit.cpp"
	"loader/ComponentReader.h" "loader/ComponentReader.cpp"
	"loader/Members.h" "loader/Members.cpp"
	"loader/ThemeReader.h" "loader/ThemeReader.cpp"
	"paint/ComponentRenderer.h" "paint/ComponentRenderer.cpp"
	"paint/Dressing.h" "paint/Dressing.cpp"
	"paint/Painter.h" "paint/Painter.cpp"
	"paint/Palette.h" "paint/Palette.cpp"
	"paint/Text.h" "paint/Text.cpp"
	"paint/CurvePages.h" "paint/CurvePages.cpp"
	"paint/CurveText.h" "paint/CurveText.cpp"
	"paint/GlyphPages.h" "paint/GlyphPages.cpp"
	"paint/TextRenderer.h" "paint/TextRenderer.cpp"
	"paint/internal/EffectQuad.h" "paint/internal/EffectQuad.cpp"
	"paint/internal/Easing.h" "paint/internal/Easing.cpp"
	"paint/internal/MenuRenderer.h" "paint/internal/MenuRenderer.cpp"
	"paint/internal/Skin.h" "paint/internal/Skin.cpp"
	"shell/FileChooser.h" "shell/FileChooser.cpp"
	"shell/GameMenu.h" "shell/GameMenu.cpp"
	"shell/Keyboard.h" "shell/Keyboard.cpp"
	"shell/Prompt.h" "shell/Prompt.cpp"
	"shell/Screen.h" "shell/Screen.cpp"
	"shell/StatisticsOverlay.h" "shell/StatisticsOverlay.cpp"
	"shell/Tooltip.h" "shell/Tooltip.cpp"
	"style/Button.h" "style/Button.cpp"
	"style/Menu.h" "style/Menu.cpp"
	"style/Property.h" "style/Property.cpp"
	"style/Resolver.h" "style/Resolver.cpp"
	"style/Style.h" "style/Style.cpp"
	"style/Theme.h" "style/Theme.cpp"
	"style/property/Color.h" "style/property/Color.cpp"
	"style/property/Image.h" "style/property/Image.cpp"
	"style/property/Number.h" "style/property/Number.cpp")

# PUBLIC: these sources compile Logger.h, and so do their consumers.
# v3dlib_render: ComponentRenderer draws onto a realtime::canvas::Canvas. The dependency is one
# way: nothing in the renderer names a ui component, and Canvas carries no vulkan of its own.
# GlyphPages.h keeps that true by taking the glyph pages' uploads as callbacks rather than
# naming a renderer.
# v3dlib_font: TextRenderer.h holds a font::GlyphStore and a font::ShapingCache.
# Boost::json: Interface.h names a boost::json::object for the theme it is handed.
# Boost::filesystem: shell/FileChooser.h names the paths it lists and hands back.
target_link_libraries(v3dlib_ui PUBLIC
	v3dlib_log
	v3dlib_render
	v3dlib_asset
	v3dlib_event
	v3dlib_font
	v3dlib_type
	Boost::json
	Boost::filesystem
	glm::glm
	EnTT::EnTT)

# PRIVATE: v3dlib_input: shell/Keyboard.cpp decodes an SDL key through input::keyName, so that
# the name a binding is written against and the name a text box receives come from one table. The
# header names neither the device nor SDL_Event beyond a forward declaration.
# SDL3::SDL3: shell/Keyboard.cpp reads SDL events and the clipboard through SDL.
# v3dlib_window: shell/Keyboard.cpp turns text input on and off on the window. Keyboard.h
# declares the window rather than including it.
target_link_libraries(v3dlib_ui PRIVATE v3dlib_input v3dlib_window SDL3::SDL3)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
