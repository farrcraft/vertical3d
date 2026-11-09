v3d_add_api_library(curve
	"Basis.h" "Basis.cxx"
	"Bezier.h"
	"BezierPoint.h"
	"CatmullRom.h"
	"Curve.h" "Curve.cxx"
	"Curves.h" "Curves.cxx"
	"Nurbs.h"
	"Poly.h"
	"Segment.h"
	"Spline.h" "Spline.cxx"
	"internal/Neighbours.h"
)

# PUBLIC: every header here holds glm values.
target_link_libraries(v3dlib_curve PUBLIC glm::glm)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
