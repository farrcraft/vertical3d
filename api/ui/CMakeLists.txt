v3d_add_api_library(ui
 "component/Button.h" "component/Button.cpp"
 "Component.h" "Component.cpp"
 "component/Icon.h" "component/Icon.cpp"
 "component/Label.h" "component/Label.cpp"
 "Overlay.h" "Overlay.cpp"
"component/Dialog.h"
"component/Frame.h"
"component/HorizontalBox.h" "component/HorizontalBox.cpp"
"component/InputBox.h"
"component/RadioButton.h"
"component/Scrollbar.h"
"component/SelectList.h"
"component/Spinner.h"
"component/TabBar.h"
"component/TabPage.h"
"component/TextBox.h"
"component/ToolTip.h"
"component/Toolbar.h" "component/Toolbar.cpp"
"component/VerticalBox.h" "component/VerticalBox.cpp"
"Container.h" "Container.cpp"
"style/Button.h"
"Style.h" "Style.cpp"
"style/Property.h" "style/Property.cpp"
"style/property/Color.h" "style/property/Color.cpp"
"style/property/Font.h" "style/property/Font.cpp"
"style/property/Number.h" "style/property/Number.cpp"
"style/property/Image.h" "style/property/Image.cpp"
"style/Theme.h" "style/Theme.cpp"
"style/Button.cpp"
"style/Menu.h" "style/Menu.cpp"
"component/CheckBox.h"
"component/menu/PopupMenu.h"
"component/menu/RadialMenu.h"
"component/menu/MenuBar.h"
"component/menu/Menu.h" "component/menu/Menu.cpp"
"component/menu/MenuItem.h" "component/menu/MenuItem.cpp"
"ComponentRenderer.h" "ComponentRenderer.cpp"
"GameMenu.h" "GameMenu.cpp"
"StatisticsOverlay.h" "StatisticsOverlay.cpp"
"TextRenderer.h" "TextRenderer.cpp"
"Engine.h" "Engine.cpp" "component/menu/Type.h" "component/Type.h" "component/menu/MenuBar.cpp" "Navigation.h")

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
target_link_libraries(v3dlib_ui PRIVATE v3dlib_image)

target_compile_options(v3dlib_ui PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
