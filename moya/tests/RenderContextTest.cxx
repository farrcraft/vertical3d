#include <boost/test/unit_test.hpp>

#include "../libmoya/RenderContext.h"

BOOST_AUTO_TEST_CASE( renderContext_test )
{
	v3D::Moya::RenderContext rc;
	unsigned int bucket_width = 0;
	unsigned int bucket_height = 0;
	unsigned int grid_size = 0;

	// test default constructor values
	bucket_width = rc.bucketWidth();
	BOOST_CHECK_EQUAL(bucket_width, 16);

	bucket_height = rc.bucketHeight();
	BOOST_CHECK_EQUAL(bucket_height, 16);

	grid_size = rc.gridSize();
	BOOST_CHECK_EQUAL(grid_size, 256);

	unsigned int image_width = 0;
	unsigned int image_height = 0;
	float aspect = 0.0f;

	image_width = rc.imageWidth();
	BOOST_CHECK_EQUAL(image_width, 320);

	image_height = rc.imageHeight();
	BOOST_CHECK_EQUAL(image_height, 240);

	aspect = rc.pixelAspect();
	BOOST_CHECK_EQUAL(aspect, 1.0f);

	// test setting image resolution
	rc.imageResolution(1024, 768, 1.3f);

	image_width = rc.imageWidth();
	BOOST_CHECK_EQUAL(image_width, 1024);

	image_height = rc.imageHeight();
	BOOST_CHECK_EQUAL(image_height, 768);

	aspect = rc.pixelAspect();
	BOOST_CHECK_EQUAL(aspect, 1.3f);

	float shading_rate = 0.0f;
	shading_rate = rc.shadingRate();
	BOOST_CHECK_EQUAL(shading_rate, 1.0f);

	// test coorindate system transforms

	v3D::Matrix4 transform(3.0f);

	rc.setTransform(transform);
	// save the transform as the world coordinate system
	rc.saveCoordinateSystem("world");
	v3D::Matrix4 world;
	world = rc.coordinateSystem("world");
	// test that we got back the same transform that we set
	BOOST_CHECK_EQUAL((transform == world), true);
	// push the transform onto the stack
	rc.pushTransform();
	// active transform should be identity
	rc.setIdentityTransform();
	// world coordinate system is overwritten with identity transform
	rc.saveCoordinateSystem("world");
	world = rc.coordinateSystem("world");
	v3D::Matrix4 identity;
	BOOST_CHECK_EQUAL((world == identity), true);
}
