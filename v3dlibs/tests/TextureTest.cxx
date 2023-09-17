#include <boost/test/unit_test.hpp>

#include "../image/Texture.h"

BOOST_AUTO_TEST_CASE( texture_test )
{
    v3D::Texture texture1;
    BOOST_CHECK_EQUAL(texture1.isnull(), true);
    BOOST_CHECK_EQUAL(texture1.wrap(), false);

    boost::shared_ptr<v3D::Image> image(new v3D::Image(2, 2, 24));
    v3D::Texture texture2(image);
    BOOST_CHECK_EQUAL(texture2.isnull(), false);
    BOOST_CHECK_EQUAL(texture2.width(), 2);
    BOOST_CHECK_EQUAL(texture2.height(), 2);

    texture2.wrap(true);
    BOOST_CHECK_EQUAL(texture2.wrap(), true);

    texture2.id(3);
    BOOST_CHECK_EQUAL(texture2.id(), 3);

    BOOST_CHECK_EQUAL((texture1 == texture2), false);
    texture1 = texture2;
    BOOST_CHECK_EQUAL((texture1 == texture2), true);

    v3D::Texture texture3(texture2);
    BOOST_CHECK_EQUAL((texture3 == texture2), true);
}