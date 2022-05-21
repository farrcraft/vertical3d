AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I .

lib_LTLIBRARIES = \
libv3dinput-@VERTICAL3D_API_VERSION@.la

libv3dinput_@VERTICAL3D_API_VERSION@_la_SOURCES = \
KeyboardDevice.cxx \
MouseDevice.cxx

libv3dinput_@VERTICAL3D_API_VERSION@_la_LDFLAGS = -version-info $(VERTICAL3D_SO_VERSION) $(GCOV_LDFLAGS)

library_includedir=$(includedir)/vertical3d-$(VERTICAL3D_API_VERSION)/vertical3d/input

library_include_HEADERS= \
InputDevice.h \
KeyboardDevice.h \
KeyboardEventListener.h \
MouseDevice.h \
MouseEventListener.h

AM_CPPFLAGS = $(DEPS_CFLAGS) $(BOOST_CPPFLAGS) $(GCOV_CFLAGS)
