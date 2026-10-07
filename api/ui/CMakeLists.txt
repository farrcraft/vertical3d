v3d_add_api_library(ui
	"Arranger.h" "Arranger.cpp"
	"Component.h" "Component.cpp"
	"Container.h" "Container.cpp"
	"DrawOrder.h"
	"Engine.h" "Engine.cpp"
	"Image.h" "Image.cpp"
	"Immediate.h" "Immediate.cpp"
	"Layout.h" "Layout.cpp"
	"Length.h" "Length.cpp"
	"Loader.h" "Loader.cpp"
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
	"component/menu/MenuBar.h" "component/menu/MenuBar.cpp"
	"component/menu/MenuItem.h" "component/menu/MenuItem.cpp"
	"component/menu/Type.h"
	"input/Command.h" "input/Command.cpp"
	"input/Cursor.h" "input/Cursor.cpp"
	"input/Keys.h" "input/Keys.cpp"
	"paint/ComponentRenderer.h" "paint/ComponentRenderer.cpp"
	"paint/Dressing.h" "paint/Dressing.cpp"
	"paint/Painter.h" "paint/Painter.cpp"
	"paint/Text.h" "paint/Text.cpp"
	"paint/TextRenderer.h" "paint/TextRenderer.cpp"
	"shell/FileChooser.h" "shell/FileChooser.cpp"
	"shell/GameMenu.h" "shell/GameMenu.cpp"
	"shell/Keyboard.h" "shell/Keyboard.cpp"
	"shell/Screen.h" "shell/Screen.cpp"
	"shell/StatisticsOverlay.h" "shell/StatisticsOverlay.cpp"
	"style/Button.h" "style/Button.cpp"
	"style/Menu.h" "style/Menu.cpp"
	"style/Property.h" "style/Property.cpp"
	"style/Resolver.h" "style/Resolver.cpp"
	"style/Style.h" "style/Style.cpp"
	"style/Theme.h" "style/Theme.cpp"
	"style/property/Color.h" "style/property/Color.cpp"
	"style/property/Font.h" "style/property/Font.cpp"
	"style/property/Image.h" "style/property/Image.cpp"
	"style/property/Number.h" "style/property/Number.cpp")

# PUBLIC: these sources compile Logger.h, and so do their consumers.
# v3dlib_render: ComponentRenderer draws onto a realtime::Canvas. The dependency is one way -
# nothing in the renderer knows about a ui component - and Canvas carries no vulkan of its own.
# TextRenderer.h keeps that true by taking the atlas upload as a callback rather than naming
# a renderer.
# v3dlib_font: TextRenderer.h holds a TextureFontCache and a TextureTextBuffer::Markup.
# v3dlib_image: TextRenderer.h holds the image::Image its atlas is uploaded from.
# Boost::json: Engine.h names a boost::json::object for the theme it is handed.
# Boost::filesystem: shell/FileChooser.h names the paths it lists and hands back.
target_link_libraries(v3dlib_ui PUBLIC
	v3dlib_log
	v3dlib_render
	v3dlib_asset
	v3dlib_event
	v3dlib_font
	v3dlib_image
	v3dlib_type
	Boost::json
	Boost::filesystem
	glm::glm
	EnTT::EnTT)

# PRIVATE: v3dlib_input: shell/Keyboard.cpp decodes an SDL key through input::keyName, so that
# the name a binding is written against and the name a text box answers come from one table. The
# header names neither the device nor SDL_Event beyond a forward declaration.
# SDL3::SDL3: shell/Keyboard.cpp reads SDL events and the clipboard through SDL.
target_link_libraries(v3dlib_ui PRIVATE v3dlib_input SDL3::SDL3)

target_compile_options(v3dlib_ui PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
