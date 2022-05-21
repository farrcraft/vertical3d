AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I .

lib_LTLIBRARIES = \
libv3dgui-@VERTICAL3D_API_VERSION@.la

libv3dgui_@VERTICAL3D_API_VERSION@_la_SOURCES = \
InputEventAdapter.cxx

libv3dgui_@VERTICAL3D_API_VERSION@_la_LDFLAGS = -version-info $(VERTICAL3D_SO_VERSION) $(GCOV_LDFLAGS)

library_includedir=$(includedir)/vertical3d-$(VERTICAL3D_API_VERSION)/vertical3d/gui

library_include_HEADERS= \
InputEventAdapter.h

AM_CPPFLAGS = $(DEPS_CFLAGS) $(BOOST_CPPFLAGS) $(GCOV_CFLAGS)
