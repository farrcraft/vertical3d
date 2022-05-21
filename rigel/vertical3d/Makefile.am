bin_PROGRAMS = vertical3d
vertical3d_SOURCES = \
	ConstructionPlane.h \
	ConstructionPlane.cxx \
	RenderView.h \
	RenderView.cxx \
	ViewLayout.h \
	ViewLayout.cxx \
	ViewPort.h \
	ViewPort.cxx \
	Window.h \
	Window.cxx \
	ArcBall.h \
	ArcBall.cxx \
	commands/CreatePolyCommandSet.h \
	commands/CreatePolyCommandSet.cxx \
	commands/PolyTools.h \
	commands/PolyTools.cxx \
	commands/ProjectCommandSet.h \
	commands/ProjectCommandSet.cxx \
	commands/TransformCommandSet.h \
	commands/TransformCommandSet.cxx \
	commands/ViewPortCommandSet.h \
	commands/ViewPortCommandSet.cxx \
	manipulators/TransformManipulator.h \
	manipulators/TransformManipulator.cxx \
	manipulators/TranslateManipulator.h \
	manipulators/TranslateManipulator.cxx \
	manipulators/RotateManipulator.h \
	manipulators/RotateManipulator.cxx \
	manipulators/ScaleManipulator.h \
	manipulators/ScaleManipulator.cxx \
	vertical3d.cxx

# set the include path found by configure
INCLUDES= $(all_includes) \
	-DVERTICAL3D_SHARED_PATH=\""$(VERTICAL3D_SHARED_DIR)/"\" \
	`pkg-config --cflags gtkglextmm-1.2` \
	`pkg-config libxml++-2.6 --cflags`

# the library search path.
vertical3d_LDFLAGS = \
	$(all_libraries) \
	`pkg-config --libs gtkglextmm-1.2` \
	`pkg-config libxml++-2.6 --libs` \
	../libv3dtypes/libv3dtypes.la \
	../libv3dgraph/libv3dgraph.la \
	../libv3dcore/libv3dcore.la \
	../libv3dcommand/libv3dcommand.la \
	../libmoya/libmoya.la
