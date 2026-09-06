/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include "../Json.h"
#include "../Manager.h"
#include "../Type.h"

namespace {

boost::shared_ptr<v3d::asset::Json> document(const std::string& name) {
    v3d::asset::Manager assets("data", boost::make_shared<v3d::log::Logger>());
    return boost::dynamic_pointer_cast<v3d::asset::Json>(assets.loadTypeFromExt(name));
}

};  // namespace

BOOST_AUTO_TEST_CASE(json_document_test) {
    auto asset = document("document.json");
    BOOST_TEST(static_cast<bool>(asset));

    auto const& doc = asset->document();
    BOOST_TEST(doc.contains("name"));
    BOOST_TEST(boost::json::value_to<std::string>(doc.at("name")) == "fixture");
    BOOST_TEST(doc.at("count").as_int64() == 3);
    BOOST_TEST(doc.at("items").as_array().size() == 3u);
    BOOST_TEST(doc.at("nested").as_object().at("flag").as_bool());
}

/**
 * A document the parser rejects is no asset at all rather than an empty one, so a consumer
 * that checks the pointer never reads a half-parsed object.
 **/
BOOST_AUTO_TEST_CASE(json_malformed_document_test) {
    BOOST_TEST(!document("malformed.json"));
}

/**
 * The loader catches its own exceptions, so a file that is not there comes back null - which
 * is what Config::load reads to decide a config is missing.
 **/
BOOST_AUTO_TEST_CASE(json_missing_document_test) {
    BOOST_TEST(!document("absent.json"));
}

/**
 * A top level array parses but is not an object, and as_object() throws on it. The loader
 * catches that too, so the rejection looks like any other.
 **/
BOOST_AUTO_TEST_CASE(json_non_object_document_test) {
    BOOST_TEST(!document("array.json"));
}
