AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I .

lib_LTLIBRARIES = \
libv3dimage-@VERTICAL3D_API_VERSION@.la

libv3dimage_@VERTICAL3D_API_VERSION@_la_SOURCES = \
Image.cxx \
ImageReader.cxx \
readers/BMPReader.cxx \
readers/TGAReader.cxx \
readers/PNGReader.cxx \
readers/JPEGReader.cxx \
ImageWriter.cxx \
writers/BMPWriter.cxx \
writers/TGAWriter.cxx \
writers/JPEGWriter.cxx \
writers/PNGWriter.cxx \
ImageFactory.cxx \
Texture.cxx

libv3dimage_@VERTICAL3D_API_VERSION@_la_LDFLAGS = -version-info $(VERTICAL3D_SO_VERSION) $(GCOV_LDFLAGS)

library_includedir=$(includedir)/vertical3d-$(VERTICAL3D_API_VERSION)/vertical3d/image

library_include_HEADERS= \
Image.h \
BMP.h \
ImageReader.h \
readers/BMPReader.h \
readers/TGAReader.h \
readers/PNGReader.h \
readers/JPEGReader.h \
ImageWriter.h \
writers/BMPWriter.h \
writers/TGAWriter.h \
writers/JPEGWriter.h \
writers/PNGWriter.h \
ImageFactory.h \
Texture.h

AM_CPPFLAGS = $(DEPS_CFLAGS) $(BOOST_CPPFLAGS) $(GCOV_CFLAGS) $(libpng_CFLAGS)
