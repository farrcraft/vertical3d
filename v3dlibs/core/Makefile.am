AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I .

lib_LTLIBRARIES = \
libv3dcore-@VERTICAL3D_API_VERSION@.la

libv3dcore_@VERTICAL3D_API_VERSION@_la_SOURCES = \
Scene.cxx \
CreatePolyCommandSet.cxx

libv3dcore_@VERTICAL3D_API_VERSION@_la_LDFLAGS = -version-info $(VERTICAL3D_SO_VERSION) $(GCOV_LDFLAGS)

library_includedir=$(includedir)/vertical3d-$(VERTICAL3D_API_VERSION)/vertical3d/core

library_include_HEADERS= \
Scene.h \
SceneVisitor.h \
CreatePolyCommandSet.h

AM_CPPFLAGS = $(DEPS_CFLAGS) $(BOOST_CPPFLAGS) $(GCOV_CFLAGS)
