AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I .

lib_LTLIBRARIES = \
libv3daudio-@VERTICAL3D_API_VERSION@.la

libv3daudio_@VERTICAL3D_API_VERSION@_la_SOURCES = \
AudioClip.cxx \
SoundEngine.cxx

libv3daudio_@VERTICAL3D_API_VERSION@_la_LDFLAGS = -version-info $(VERTICAL3D_SO_VERSION) $(GCOV_LDFLAGS)

library_includedir=$(includedir)/vertical3d-$(VERTICAL3D_API_VERSION)/vertical3d/audio

library_include_HEADERS= \
AudioClip.h \
SoundEngine.h

AM_CPPFLAGS = $(DEPS_CFLAGS) $(BOOST_CPPFLAGS) $(GCOV_CFLAGS) $(openal_CFLAGS) $(freealut_CFLAGS)
