AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I .

lib_LTLIBRARIES = \
libv3dfont-@VERTICAL3D_API_VERSION@.la

libv3dfont_@VERTICAL3D_API_VERSION@_la_SOURCES = \
Font2D.cxx \
FontCache.cxx

libv3dfont_@VERTICAL3D_API_VERSION@_la_LDFLAGS = -version-info $(VERTICAL3D_SO_VERSION) $(GCOV_LDFLAGS)

library_includedir=$(includedir)/vertical3d-$(VERTICAL3D_API_VERSION)/vertical3d/font

library_include_HEADERS= \
Font2D.h \
FontCache.h

AM_CPPFLAGS = $(DEPS_CFLAGS) $(BOOST_CPPFLAGS) $(GCOV_CFLAGS) $(freetype2_CFLAGS)
