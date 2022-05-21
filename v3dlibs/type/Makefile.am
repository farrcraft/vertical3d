AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I .

lib_LTLIBRARIES = \
libv3dtypes-@VERTICAL3D_API_VERSION@.la

libv3dtypes_@VERTICAL3D_API_VERSION@_la_SOURCES = \
AABBox.cxx \
ArcBall.cxx \
Bound2D.cxx \
Camera.cxx \
CameraProfile.cxx \
Color3.cxx \
Color4.cxx \
Matrix4.cxx \
Quaternion.cxx \
Vector2.cxx \
Vector3.cxx

libv3dtypes_@VERTICAL3D_API_VERSION@_la_LDFLAGS = -version-info $(VERTICAL3D_SO_VERSION) $(GCOV_LDFLAGS)

library_includedir=$(includedir)/vertical3d-$(VERTICAL3D_API_VERSION)/vertical3d/3dtypes

library_include_HEADERS= \
3dtypes.h \
AABBox.h \
ArcBall.h \
Bound2D.h \
Camera.h \
CameraProfile.h \
Color3.h \
Color4.h \
Matrix4.h \
Quaternion.h \
Vector.h \
Vector2.h \
Vector3.h

AM_CPPFLAGS = $(DEPS_CFLAGS) $(BOOST_CPPFLAGS) $(GCOV_CFLAGS)
