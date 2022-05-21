AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I .

lib_LTLIBRARIES = \
libv3dbrep-@VERTICAL3D_API_VERSION@.la

libv3dbrep_@VERTICAL3D_API_VERSION@_la_SOURCES = \
BRep.cxx \
Face.cxx \
HalfEdge.cxx \
Vertex.cxx

libv3dbrep_@VERTICAL3D_API_VERSION@_la_LDFLAGS = -version-info $(VERTICAL3D_SO_VERSION) $(GCOV_LDFLAGS)

library_includedir=$(includedir)/vertical3d-$(VERTICAL3D_API_VERSION)/vertical3d/brep

library_include_HEADERS= \
BRep.h \
Face.h \
HalfEdge.h \
Vertex.h

AM_CPPFLAGS = $(DEPS_CFLAGS) $(BOOST_CPPFLAGS) $(GCOV_CFLAGS)
