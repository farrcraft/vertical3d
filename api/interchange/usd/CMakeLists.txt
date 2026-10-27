v3d_add_api_library(interchange_usd
	"Usd.h" "Usd.cxx")

# PUBLIC: the reader returns an interchange::Scene. glm, for the scene's points and values.
target_link_libraries(v3dlib_interchange_usd PUBLIC v3dlib_interchange glm::glm)

# PRIVATE: the reader places nodes by type::Transform and builds curve::Curve values, and only
# its source names OpenUSD.
target_link_libraries(v3dlib_interchange_usd PRIVATE v3dlib_curve v3dlib_type usd usdGeom usdLux)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
