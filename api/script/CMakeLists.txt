v3d_add_api_library(script
	"Runtime.h" "Runtime.cxx"
	"Strip.h" "Strip.cxx"
	"internal/Scanner.h" "internal/Scanner.cxx"
	"internal/Eraser.h" "internal/Eraser.cxx" "internal/EraserExpressions.cxx" "internal/EraserTypes.cxx"
)

# PUBLIC: Runtime.h passes JSON values to and from the functions a script calls. QuickJS is
# PRIVATE, since no public header includes it.
target_link_libraries(v3dlib_script PUBLIC Boost::json PRIVATE qjs)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
