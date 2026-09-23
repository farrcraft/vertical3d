v3d_add_api_library(ui
	"Arranger.h" "Arranger.cpp"
	"Component.h" "Component.cpp"
	"Container.h" "Container.cpp"
	"Engine.h" "Engine.cpp"
	"Image.h" "Image.cpp"
	"Immediate.h" "Immediate.cpp"
	"Layout.h" "Layout.cpp"
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
	"shell/GameMenu.h" "shell/GameMenu.cpp"
	"shell/Keyboard.h" "shell/Keyboard.cpp"
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
# TextRenderer.h keeps that true by forward declaring the QuadRenderer it uploads an atlas
# through, rather than including it.
# v3dlib_font: TextRenderer.h holds a TextureFontCache and a TextureTextBuffer::Markup.
# Boost::json: Engine.h names a boost::json::object for the theme it is handed.
target_link_libraries(v3dlib_ui PUBLIC
	v3dlib_log
	v3dlib_render
	v3dlib_asset
	v3dlib_event
	v3dlib_font
	v3dlib_type
	Boost::json
	glm::glm
	EnTT::EnTT)

# PRIVATE: TextRenderer packs its glyphs into an image::TextureAtlas, which no ui header names.
# v3dlib_input: shell/Keyboard.cpp decodes an SDL key through input::keyName, so that the name
# a binding is written against and the name a text box answers come from one table. The header
# names neither the device nor SDL_Event beyond a forward declaration.
target_link_libraries(v3dlib_ui PRIVATE v3dlib_image v3dlib_input)

target_compile_options(v3dlib_ui PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
