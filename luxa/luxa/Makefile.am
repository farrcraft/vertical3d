AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I .

lib_LTLIBRARIES = \
libluxa-@LUXA_API_VERSION@.la

libluxa_@LUXA_API_VERSION@_la_SOURCES = \
Button.cxx \
ButtonStyle.cxx \
ColorStyleProperty.cxx \
Component.cxx \
ComponentManager.cxx \
ComponentRenderer.cxx \
Container.cxx \
FontStyleProperty.cxx \
HorizontalBox.cxx \
Icon.cxx \
ImageStyleProperty.cxx \
Label.cxx \
Menu.cxx \
MenuItem.cxx \
MenuStack.cxx \
MenuStyle.cxx \
Overlay.cxx \
Style.cxx \
StyleProperty.cxx \
Theme.cxx \
UILoader.cxx \
VerticalBox.cxx

libluxa_@LUXA_API_VERSION@_la_LDFLAGS = -version-info $(LUXA_SO_VERSION) $(GCOV_LDFLAGS)

library_includedir=$(includedir)/luxa-$(LUXA_API_VERSION)/luxa

library_include_HEADERS= \
Button.h \
ButtonStyle.h \
CheckBox.h \
ColorStyleProperty.h \
Component.h \
ComponentManager.h \
ComponentRenderer.h \
Container.h \
Dialog.h \
FontStyleProperty.h \
Frame.h \
HorizontalBox.h \
Icon.h \
ImageStyleProperty.h \
InputBox.h \
Label.h \
Menu.h \
MenuBar.h \
MenuItem.h \
MenuStack.h \
MenuStyle.h \
Overlay.h \
PopupMenu.h \
RadialMenu.h \
RadioButton.h \
Scrollbar.h \
SelectList.h \
Spinner.h \
Style.h \
StyleProperty.h \
TabBar.h \
TabPage.h \
TextBox.h \
Theme.h \
ToolTip.h \
UILoader.h \
VerticalBox.h \
Window.h

AM_CPPFLAGS = $(DEPS_CFLAGS) $(BOOST_CPPFLAGS) $(GCOV_CFLAGS) $(V3D_CPPFLAGS)
