#include <boost/test/unit_test.hpp>

#include "../3dtypes/ArcBall.h"

BOOST_AUTO_TEST_CASE( arcball_test )
{
	v3D::ArcBall ball;

	// bounds
	ball.bounds(0.5f, 0.5f);

	// click
	v3D::Vector2 point(1.0f, 1.0f);
	ball.click(point);

	// map

	// drag

}