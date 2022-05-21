AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I .

lib_LTLIBRARIES = \
libv3dhookah-@VERTICAL3D_API_VERSION@.la \
libv3dhookah_sdl-@VERTICAL3D_API_VERSION@.la \
libv3dhookah_fltk-@VERTICAL3D_API_VERSION@.la

libv3dhookah_@VERTICAL3D_API_VERSION@_la_SOURCES = \
Window.cxx

libv3dhookah_fltk_@VERTICAL3D_API_VERSION@_la_SOURCES = \
drivers/FLTKDriver.cxx \
drivers/FLTKKeyboard.cxx \
drivers/FLTKMouse.cxx \
drivers/FLTKWindow.cxx

libv3dhookah_sdl_@VERTICAL3D_API_VERSION@_la_SOURCES = \
drivers/SDLDriver.cxx \
drivers/SDLMouse.cxx \
drivers/SDLKeyboard.cxx \
drivers/SDLWindow.cxx

libv3dhookah_@VERTICAL3D_API_VERSION@_la_LDFLAGS = -version-info $(VERTICAL3D_SO_VERSION) $(GCOV_LDFLAGS)
libv3dhookah_fltk_@VERTICAL3D_API_VERSION@_la_LDFLAGS = -version-info $(VERTICAL3D_SO_VERSION) $(GCOV_LDFLAGS)
libv3dhookah_sdl_@VERTICAL3D_API_VERSION@_la_LDFLAGS = -version-info $(VERTICAL3D_SO_VERSION) $(GCOV_LDFLAGS)

library_includedir=$(includedir)/vertical3d-$(VERTICAL3D_API_VERSION)/vertical3d/hookah

library_include_HEADERS= \
Hookah.h \
Window.h

librarydrivers_includedir=$(includedir)/vertical3d-$(VERTICAL3D_API_VERSION)/vertical3d/hookah/drivers

librarydrivers_include_HEADERS= \
drivers/FLTKKeyboard.h \
drivers/FLTKMouse.h \
drivers/FLTKWindow.h \
drivers/SDLMouse.h \
drivers/SDLKeyboard.h \
drivers/SDLWindow.h

AM_CPPFLAGS = $(DEPS_CFLAGS) $(BOOST_CPPFLAGS) $(GCOV_CFLAGS) $(libpng_CFLAGS)
