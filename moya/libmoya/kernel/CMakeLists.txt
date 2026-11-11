# moya's kernel code: what the CPU and every GPU back end compile from one source. This target
# compiles it under the rules a GPU compiler imposes, so a breach fails on the CPU first.
# Kernel.cxx includes every header in this directory, so each one is compiled here as well as
# wherever else it is included. scripts/kernel.ts checks what no compiler flag can, such as a
# virtual function or an allocation.
add_library("v3dlib_moya_kernel"
	"Array.h"
	"camera/Camera.h"
	"film/FilterSample.h"
	"film/PixelFilter.h"
	"film/Pixels.h"
	"bsdf/AtSurface.h"
	"bsdf/Bsdf.h"
	"bsdf/Diffuse.h"
	"bsdf/Lobe.h"
	"bsdf/Sample.h"
	"integrate/Dimensions.h"
	"integrate/Paths.h"
	"integrate/PathState.h"
	"integrate/Settings.h"
	"light/Bounds.h"
	"light/Dome.h"
	"light/Elliptic.h"
	"light/Emission.h"
	"light/Image.h"
	"light/Light.h"
	"light/link/Links.h"
	"light/link/LinkStep.h"
	"light/Lights.h"
	"light/LightSample.h"
	"light/Picked.h"
	"light/Sampling.h"
	"light/SphericalEllipse.h"
	"light/SphericalRectangle.h"
	"light/SphericalTriangle.h"
	"light/Tree.h"
	"geometry/Between.h"
	"geometry/BruteForce.h"
	"geometry/Hit.h"
	"geometry/Key.h"
	"geometry/Motion.h"
	"geometry/Offset.h"
	"geometry/Placed.h"
	"geometry/PlacedShapes.h"
	"geometry/Placement.h"
	"geometry/primitive/Curves.h"
	"geometry/primitive/Polynomial.h"
	"geometry/primitive/Quadratic.h"
	"geometry/primitive/Quadric.h"
	"geometry/primitive/RayFrame.h"
	"geometry/primitive/ShapeHit.h"
	"geometry/primitive/Shapes.h"
	"geometry/primitive/Shear.h"
	"geometry/primitive/Triangle.h"
	"geometry/primitive/TriangleHit.h"
	"geometry/Ray.h"
	"geometry/Sampled.h"
	"geometry/Trace.h"
	"geometry/Traversal.h"
	"Launch.h"
	"Qualifiers.h"
	"sample/Hash.h"
	"sample/Sobol.h"
	"sample/ZSobol.h"
	"spectrum/Acescg.h"
	"spectrum/CieD65.h"
	"spectrum/CieDaylight.h"
	"spectrum/CieObserver.h"
	"spectrum/Colorimetry.h"
	"spectrum/Daylight.h"
	"spectrum/Rgb.h"
	"spectrum/RgbSigmoid.h"
	"spectrum/RgbTable.h"
	"spectrum/Spectrum.h"
	"spectrum/Visible.h"
	"spectrum/Wavelengths.h"
	"surface/Frame.h"
	"surface/Geometry.h"
	"surface/Instance.h"
	"surface/Material.h"
	"surface/Primvar.h"
	"surface/ShadingPoint.h"
	"surface/Surfaces.h"
	"view/HitChannels.h"
	"view/HitView.h"
	"Kernel.cxx"
)

# The newest standard both HIP and nvcc accept, which is older than the tree's.
set_target_properties("v3dlib_moya_kernel" PROPERTIES CXX_STANDARD 20 V3D_KERNEL ON)

# Kernel code includes its headers from the root of the tree, as the api libraries do. Its headers
# name glm's vectors.
target_include_directories("v3dlib_moya_kernel" PUBLIC "${V3D_ROOT}")
target_link_libraries("v3dlib_moya_kernel" PUBLIC glm::glm)

# PUBLIC: the colour type is fixed when moya is compiled, so every source that compiles a kernel
# header sees the same one.
if(V3D_MOYA_RGB)
	target_compile_definitions("v3dlib_moya_kernel" PUBLIC MOYA_RGB)
endif()

# No exceptions and no RTTI, which a GPU compiler does not support. The top-level build leaves
# /EHsc off a target that sets V3D_KERNEL. _HAS_EXCEPTIONS=0 keeps MSVC's standard headers from
# throwing, which they would do without unwind semantics.
#
# PUBLIC: no multiply and add is fused into one rounding, in this target or in any source that
# compiles a kernel header, so MSVC, Clang and GCC round alike. MSVC's cl fuses only under
# /fp:contract or /fp:fast, which the tree never gives. clang-cl takes Clang's flag through
# /clang:.
if(MSVC)
	target_compile_options("v3dlib_moya_kernel" PRIVATE /GR- /utf-8)
	target_compile_definitions("v3dlib_moya_kernel" PRIVATE _HAS_EXCEPTIONS=0)
	if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		target_compile_options("v3dlib_moya_kernel" PUBLIC /clang:-ffp-contract=off)
	endif()
else()
	target_compile_options("v3dlib_moya_kernel" PRIVATE -fno-exceptions -fno-rtti)
	target_compile_options("v3dlib_moya_kernel" PUBLIC -ffp-contract=off)
endif()
