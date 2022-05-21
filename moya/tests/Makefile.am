AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I .

bin_PROGRAMS = unit_tests
unit_tests_SOURCES = \
TestMain.cxx \
RenderContextTest.cxx \
ReyesPrimitiveTest.cxx \
VertexTest.cxx \
RendererTest.cxx \
PolygonTest.cxx

AM_CPPFLAGS = $(DEPS_CFLAGS) $(BOOST_CPPFLAGS) $(GCOV_CFLAGS) $(V3D_CPPFLAGS)
AM_LDFLAGS = $(BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS) $(GCOV_LDFLAGS) $(V3D_TYPES_LDFLAGS) \
$(BOOST_SIGNALS_LDFLAGS) $(BOOST_SYSTEM_LDFLAGS) $(BOOST_FILESYSTEM_LDFLAGS)

unit_tests_LDADD = $(DEPS_LIBS) \
$(BOOST_UNIT_TEST_FRAMEWORK_LIBS) $(BOOST_SIGNALS_LIBS) $(BOOST_SYSTEM_LIBS) $(BOOST_FILESYSTEM_LIBS) \
$(libpng_LIBS) $(JPEG_LIBS) $(liblog4cxx_LIBS) $(freetype2_LIBS) $(V3D_TYPES_LIBS) \
$(top_builddir)/src/libmoya/.libs/libmoya-@MOYA_API_VERSION@.la
