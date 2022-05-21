AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I .

lib_LTLIBRARIES = \
libv3dgl-@VERTICAL3D_API_VERSION@.la

libv3dgl_@VERTICAL3D_API_VERSION@_la_SOURCES = \
GLTexture.cxx \
GLFontRenderer.cxx

libv3dgl_@VERTICAL3D_API_VERSION@_la_LDFLAGS = -version-info $(VERTICAL3D_SO_VERSION) $(GCOV_LDFLAGS)

library_includedir=$(includedir)/vertical3d-$(VERTICAL3D_API_VERSION)/vertical3d/gl

library_include_HEADERS= \
GLTexture.h \
GLFontRenderer.h

AM_CPPFLAGS = $(DEPS_CFLAGS) $(BOOST_CPPFLAGS) $(GCOV_CFLAGS)
