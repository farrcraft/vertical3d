AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I .

lib_LTLIBRARIES = \
libmoya-@MOYA_API_VERSION@.la

libmoya_@MOYA_API_VERSION@_la_SOURCES = \
Bucket.cxx \
FrameBuffer.cxx \
Frustum.cxx \
MicroPolygon.cxx \
MicroPolygonGrid.cxx \
Plane.cxx \
Polygon.cxx \
RenderContext.cxx \
RenderMan.cxx \
Renderer.cxx \
ReyesPrimitive.cxx \
Vertex.cxx

libmoya_@MOYA_API_VERSION@_la_LDFLAGS = -version-info $(MOYA_SO_VERSION) $(GCOV_LDFLAGS) $(V3D_TYPES_LDFLAGS)

library_includedir=$(includedir)/moya-$(MOYA_API_VERSION)/moya

library_include_HEADERS= \
Bucket.h \
FrameBuffer.h \
Frustum.h \
MicroPolygon.h \
MicroPolygonGrid.h \
Plane.h \
Polygon.h \
RenderContext.h \
RenderMan.h \
Renderer.h \
ReyesPrimitive.h \
Vertex.h

AM_CPPFLAGS = $(DEPS_CFLAGS) $(BOOST_CPPFLAGS) $(GCOV_CFLAGS) $(V3D_CPPFLAGS)
