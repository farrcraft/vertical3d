/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Migration.h>

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/json.hpp>

namespace {

/**
 * A document at a version, holding one value a step can move.
 **/
boost::json::object at(const boost::json::value& version) {
    boost::json::object document;
    document["version"] = version;
    document["name"] = "acorn";
    return document;
}

};  // namespace

BOOST_AUTO_TEST_SUITE(migration_test)

/**
 * A document already at this build's version is read as it is, whatever the chain holds.
 **/
BOOST_AUTO_TEST_CASE(a_current_document_is_untouched) {
    boost::json::object document = at(2);
    const boost::json::object before = document;
    const std::vector<v3d::asset::Migration> chain = {
        [](boost::json::object&) { return false; }
    };
    BOOST_CHECK(v3d::asset::readForward(&document, 2, chain) == v3d::asset::Reading::Current);
    BOOST_CHECK(document == before);
}

/**
 * The steps run oldest first, each seeing the version the one before it stamped, and the
 * document ends at this build's version.
 **/
BOOST_AUTO_TEST_CASE(the_steps_run_in_order) {
    std::vector<std::string> seen;
    const std::vector<v3d::asset::Migration> chain = {
        [&seen](boost::json::object& document) {
            seen.push_back("1 at " + boost::json::serialize(document.at("version")));
            // copied out first: inserting a key can move what at() referred to
            const boost::json::value name = document.at("name");
            document["title"] = name;
            document.erase("name");
            return true;
        },
        [&seen](boost::json::object& document) {
            seen.push_back("2 at " + boost::json::serialize(document.at("version")));
            document["title"] = "an " + std::string(document.at("title").as_string());
            return true;
        }
    };

    boost::json::object document = at(1);
    BOOST_CHECK(v3d::asset::readForward(&document, 3, chain) == v3d::asset::Reading::Migrated);
    BOOST_REQUIRE_EQUAL(seen.size(), 2u);
    BOOST_CHECK_EQUAL(seen[0], "1 at 1");
    BOOST_CHECK_EQUAL(seen[1], "2 at 2");
    BOOST_CHECK_EQUAL(document.at("version").as_int64(), 3);
    BOOST_CHECK_EQUAL(std::string(document.at("title").as_string()), "an acorn");
    BOOST_CHECK(!document.contains("name"));

    // and a document part way along runs only the steps after it
    seen.clear();
    boost::json::object later = at(2);
    later["title"] = "acorn";
    BOOST_CHECK(v3d::asset::readForward(&later, 3, chain) == v3d::asset::Reading::Migrated);
    BOOST_REQUIRE_EQUAL(seen.size(), 1u);
    BOOST_CHECK_EQUAL(seen[0], "2 at 2");
}

/**
 * A step that fails refuses the whole document, and what the steps before it changed is
 * thrown away with the copy.
 **/
BOOST_AUTO_TEST_CASE(a_failed_step_leaves_the_document_as_it_was) {
    const std::vector<v3d::asset::Migration> chain = {
        [](boost::json::object& document) {
            document["name"] = "changed";
            return true;
        },
        [](boost::json::object&) { return false; }
    };
    boost::json::object document = at(1);
    const boost::json::object before = document;
    BOOST_CHECK(v3d::asset::readForward(&document, 3, chain) == v3d::asset::Reading::Refused);
    BOOST_CHECK(document == before);
}

/**
 * A chain short of a step refuses before any step runs, and so does an empty step.
 **/
BOOST_AUTO_TEST_CASE(a_missing_step_refuses) {
    int ran = 0;
    const std::vector<v3d::asset::Migration> chain = {
        [&ran](boost::json::object&) { ++ran; return true; }
    };
    boost::json::object document = at(1);
    BOOST_CHECK(v3d::asset::readForward(&document, 3, chain) == v3d::asset::Reading::Refused);
    BOOST_CHECK_EQUAL(ran, 0);

    const std::vector<v3d::asset::Migration> hollow(1);
    BOOST_CHECK(v3d::asset::readForward(&document, 2, hollow) == v3d::asset::Reading::Refused);
}

/**
 * A version that is missing, that is not a whole number, or that is below one is refused, and
 * the document is untouched.
 **/
BOOST_AUTO_TEST_CASE(a_version_that_is_not_one_refuses) {
    const std::vector<v3d::asset::Migration> chain = {
        [](boost::json::object&) { return true; }
    };
    for (const boost::json::value& version : { boost::json::value("1"), boost::json::value(true),
            boost::json::value(1.5), boost::json::value(0), boost::json::value(-1) }) {
        boost::json::object document = at(version);
        const boost::json::object before = document;
        BOOST_CHECK(v3d::asset::readForward(&document, 2, chain) == v3d::asset::Reading::Refused);
        BOOST_CHECK(document == before);
    }

    boost::json::object unversioned = at(1);
    unversioned.erase("version");
    BOOST_CHECK(v3d::asset::readForward(&unversioned, 2, chain) == v3d::asset::Reading::Refused);
}

/**
 * A document a later build wrote is newer, and untouched.
 **/
BOOST_AUTO_TEST_CASE(a_later_document_is_newer) {
    boost::json::object document = at(5);
    const boost::json::object before = document;
    BOOST_CHECK(v3d::asset::readForward(&document, 2, {}) == v3d::asset::Reading::Newer);
    BOOST_CHECK(document == before);
}

BOOST_AUTO_TEST_SUITE_END()
