/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include "../Texture.h"

BOOST_AUTO_TEST_CASE(texture_test) {
    v3d::image::Texture texture1;
    BOOST_CHECK_EQUAL(texture1.isnull(), true);
    BOOST_CHECK_EQUAL(texture1.wrap(), false);

    boost::shared_ptr<v3d::image::Image> image = boost::make_shared<v3d::image::Image>(2, 2, 24);
    v3d::image::Texture texture2(image);
    BOOST_CHECK_EQUAL(texture2.isnull(), false);
    BOOST_CHECK_EQUAL(texture2.width(), 2u);
    BOOST_CHECK_EQUAL(texture2.height(), 2u);
    BOOST_CHECK_EQUAL((texture2.image() == image), true);

    texture2.wrap(true);
    BOOST_CHECK_EQUAL(texture2.wrap(), true);

    texture2.id(3);
    BOOST_CHECK_EQUAL(texture2.id(), 3u);

    BOOST_CHECK_EQUAL((texture1 == texture2), false);
    texture1 = texture2;
    BOOST_CHECK_EQUAL((texture1 == texture2), true);

    v3d::image::Texture texture3(texture2);
    BOOST_CHECK_EQUAL((texture3 == texture2), true);

    // release drops the image, which is what makes a texture null again
    texture3.release();
    BOOST_CHECK_EQUAL(texture3.isnull(), true);
}
