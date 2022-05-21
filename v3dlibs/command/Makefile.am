AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I .

lib_LTLIBRARIES = \
libv3dcommand-@VERTICAL3D_API_VERSION@.la

libv3dcommand_@VERTICAL3D_API_VERSION@_la_SOURCES = \
Bind.cxx \
Command.cxx \
CommandDirectory.cxx \
CommandInfo.cxx \
EventInfo.cxx \
BindLoader.cxx 
# missing header file in git repo for this file right now
#StateController.cxx

libv3dcommand_@VERTICAL3D_API_VERSION@_la_LDFLAGS = -version-info $(VERTICAL3D_SO_VERSION) $(GCOV_LDFLAGS)

library_includedir=$(includedir)/vertical3d-$(VERTICAL3D_API_VERSION)/vertical3d/command

library_include_HEADERS= \
Bind.h \
Command.h \
CommandDirectory.h \
CommandInfo.h \
EventInfo.h \
EventListener.h \
BindLoader.h

AM_CPPFLAGS = $(DEPS_CFLAGS) $(BOOST_CPPFLAGS) $(GCOV_CFLAGS)
