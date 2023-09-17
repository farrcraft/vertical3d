#include <boost/test/unit_test.hpp>

#include "../font/FontCache.h"

BOOST_AUTO_TEST_CASE( fontcache_test )
{
    v3D::FontCache cache;
    bool result = cache.load("Comic Sans", "comicsans.ttf", 12);
    BOOST_CHECK_EQUAL(result, false);

    std::string fontpath("/usr/share/fonts/corefonts/arial.ttf");
    result = cache.load("Arial", fontpath, 14);
    BOOST_CHECK_EQUAL(result, true);

    boost::shared_ptr<v3D::Font2D> font = cache.get("Helvetica");
    boost::shared_ptr<v3D::Font2D> empty_ptr;
    BOOST_CHECK_EQUAL(font, empty_ptr);

    boost::shared_ptr<v3D::Font2D> font2 = cache.get("Arial");
    BOOST_CHECK_EQUAL((font2->typeface() == fontpath), true);
}