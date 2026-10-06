/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/File.h>
#include <api/asset/Manager.h>
#include <api/asset/Type.h>
#include <api/asset/kind/Text.h>

#include <optional>
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

/**
 * readFile returns the bytes on disk, nothing for a file that will not open, and an empty
 * string for an empty file.
 **/
BOOST_AUTO_TEST_CASE(read_file_test) {
    const std::optional<std::string> plain = v3d::asset::readFile("data/plain.txt");
    BOOST_REQUIRE(plain.has_value());
    BOOST_TEST(plain.value_or("") == "one\ntwo\n");

    BOOST_TEST(!v3d::asset::readFile("data/absent.txt").has_value());

    const std::optional<std::string> empty = v3d::asset::readFile("data/empty.txt");
    BOOST_TEST(empty.value_or("not empty").empty());
}
