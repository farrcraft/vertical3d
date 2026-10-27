v3d_add_api_library(curve
	"Basis.h" "Basis.cxx"
	"Curve.h" "Curve.cxx"
	"Curves.h" "Curves.cxx"
	"Segment.h"
	"Spline.h" "Spline.cxx"
)

# PUBLIC: every header here holds glm values.
target_link_libraries(v3dlib_curve PUBLIC glm::glm)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
