/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Manager.h>
#include <api/asset/Type.h>
#include <api/asset/kind/Text.h>

#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

namespace {

boost::shared_ptr<v3d::asset::Manager> manager() {
    return boost::make_shared<v3d::asset::Manager>("data", boost::make_shared<v3d::log::Logger>());
}

};  // namespace

/**
 * The file is read as binary and sized from the stream, so the content is byte for byte what
 * is on disk - a shader source is the caller this matters to.
 **/
BOOST_AUTO_TEST_CASE(text_content_test) {
    auto asset = boost::dynamic_pointer_cast<v3d::asset::kind::Text>(
        manager()->load("plain.txt", v3d::asset::Type::Text));

    BOOST_TEST(static_cast<bool>(asset));
    BOOST_TEST(asset->content() == "one\ntwo\n");
}

/**
 * A missing file is no asset, the same as every other loader.
 **/
BOOST_AUTO_TEST_CASE(text_missing_file_test) {
    BOOST_TEST(!manager()->load("absent.txt", v3d::asset::Type::Text));
}
