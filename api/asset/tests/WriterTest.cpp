/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Writer.h>

#include <fstream>
#include <limits>
#include <sstream>
#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/json.hpp>

namespace {

/**
 * A directory of its own per case, removed with everything under it, so a case that leaves a
 * temporary behind fails the case that looks for it rather than the next run.
 **/
class Sandbox {
 public:
    explicit Sandbox(const std::string& name) :
        path_(boost::filesystem::temp_directory_path() / ("v3d_writer_" + name)) {
        boost::system::error_code ignored;
        boost::filesystem::remove_all(path_, ignored);
        boost::filesystem::create_directories(path_);
    }

    ~Sandbox() {
        boost::system::error_code ignored;
        boost::filesystem::remove_all(path_, ignored);
    }

    Sandbox(const Sandbox&) = delete;
    Sandbox& operator=(const Sandbox&) = delete;

    boost::filesystem::path file(const std::string& name) const {
        return path_ / name;
    }

 private:
    boost::filesystem::path path_;
};

std::string contents(const boost::filesystem::path& path) {
    std::ifstream file(path.c_str(), std::ios::binary);
    std::ostringstream text;
    text << file.rdbuf();
    return text.str();
}

};  // namespace

/**
 * The serializer is pure, so what it produces has to parse back to what went in.
 *
 * By value rather than by type: a whole number prints without a fractional part, so a double
 * that happens to be 2.0 reads back as an integer. Everything reading a document this writes
 * takes a number rather than a double, so that is harmless.
 **/
BOOST_AUTO_TEST_CASE(writer_round_trip_test) {
    boost::json::object root;
    root["version"] = 1;
    root["name"] = "fixture";
    root["scale"] = boost::json::array{ 1.5, 2.0, 0.25 };
    root["flag"] = true;
    root["nothing"] = boost::json::value();
    root["meshes"] = boost::json::array{
        boost::json::object{ { "edge", 4 }, { "normal", boost::json::array{ 0.0, 1.0, 0.0 } } } };

    const std::string text = v3d::asset::serializeDocument(root);
    boost::system::error_code error;
    const boost::json::value parsed = boost::json::parse(text, error);
    BOOST_REQUIRE(!error);
    BOOST_REQUIRE(parsed.is_object());

    const boost::json::object& read = parsed.as_object();
    BOOST_CHECK_EQUAL(read.at("version").to_number<int>(), 1);
    BOOST_CHECK_EQUAL(boost::json::value_to<std::string>(read.at("name")), "fixture");
    BOOST_CHECK_EQUAL(read.at("flag").as_bool(), true);
    BOOST_CHECK(read.at("nothing").is_null());

    const boost::json::array& scale = read.at("scale").as_array();
    BOOST_REQUIRE_EQUAL(scale.size(), 3u);
    BOOST_CHECK_EQUAL(scale[0].to_number<double>(), 1.5);
    BOOST_CHECK_EQUAL(scale[1].to_number<double>(), 2.0);
    BOOST_CHECK_EQUAL(scale[2].to_number<double>(), 0.25);

    const boost::json::object& mesh = read.at("meshes").as_array().at(0).as_object();
    BOOST_CHECK_EQUAL(mesh.at("edge").to_number<int>(), 4);
    BOOST_CHECK_EQUAL(mesh.at("normal").as_array().at(1).to_number<double>(), 1.0);
}

/**
 * The two cases the printer handles: a float that would otherwise print every digit of
 * the double it was widened to, and a vector that would otherwise take a line per number.
 **/
BOOST_AUTO_TEST_CASE(writer_readable_test) {
    boost::json::object root;
    root["coordinate"] = static_cast<double>(0.1f);
    BOOST_CHECK_EQUAL(v3d::asset::serializeDocument(root), "{ \"coordinate\": 0.1 }");

    boost::json::object vector;
    vector["translation"] = boost::json::array{ 1.0, 2.0, 3.0 };
    BOOST_CHECK_EQUAL(v3d::asset::serializeDocument(vector), "{ \"translation\": [1, 2, 3] }");

    // a record of scalars and vectors is one line; anything holding one is not
    boost::json::object nested;
    nested["outer"] = vector;
    BOOST_CHECK_EQUAL(v3d::asset::serializeDocument(nested),
        "{\n  \"outer\": { \"translation\": [1, 2, 3] }\n}");

    // an integer is not run through the float path
    boost::json::object counted;
    counted["count"] = 3;
    BOOST_CHECK_EQUAL(v3d::asset::serializeDocument(counted), "{ \"count\": 3 }");

    BOOST_CHECK_EQUAL(v3d::asset::serializeDocument(boost::json::object()), "{}");
    BOOST_CHECK_EQUAL(v3d::asset::serializeDocument(boost::json::array()), "[]");
}

/**
 * A double too large for a float prints as the double it is, and parses back to it.
 **/
BOOST_AUTO_TEST_CASE(writer_double_beyond_float_range_test) {
    boost::json::array numbers{ 1e300, -1e300, 1e39 };
    const std::string text = v3d::asset::serializeDocument(numbers);
    BOOST_CHECK(text.find("inf") == std::string::npos);

    boost::system::error_code error;
    const boost::json::value parsed = boost::json::parse(text, error);
    BOOST_REQUIRE(!error);
    BOOST_REQUIRE(parsed.is_array());
    const boost::json::array& read = parsed.as_array();
    BOOST_REQUIRE_EQUAL(read.size(), 3u);
    BOOST_CHECK_CLOSE(read[0].to_number<double>(), 1e300, 1e-9);
    BOOST_CHECK_CLOSE(read[1].to_number<double>(), -1e300, 1e-9);
    BOOST_CHECK_CLOSE(read[2].to_number<double>(), 1e39, 1e-9);
}

/**
 * A double inside the range of a float that no float holds exactly prints as the double it is.
 * Narrowed to a float, the first would print as 0 and the second as 123456790.
 **/
BOOST_AUTO_TEST_CASE(writer_double_no_float_holds_test) {
    const boost::json::array numbers{ 1e-50, 123456789.123 };
    BOOST_CHECK_EQUAL(v3d::asset::serializeDocument(numbers), "[1e-50, 123456789.123]");
}

/**
 * JSON has no form for infinity or NaN, so a document holding one, however deeply, is refused
 * and the file already there is untouched.
 **/
BOOST_AUTO_TEST_CASE(writer_refuses_a_non_finite_number_test) {
    const Sandbox sandbox("non_finite");
    const boost::filesystem::path path = sandbox.file("settings.json");
    BOOST_REQUIRE(v3d::asset::writeFile(path, "first"));

    boost::json::object infinite;
    infinite["scale"] = boost::json::array{ 1.0, std::numeric_limits<double>::infinity() };
    BOOST_CHECK(!v3d::asset::writeDocument(path, infinite));

    boost::json::object undefined;
    undefined["nested"] = boost::json::object{ { "value", std::numeric_limits<double>::quiet_NaN() } };
    BOOST_CHECK(!v3d::asset::writeDocument(path, undefined));

    BOOST_CHECK(!v3d::asset::writeDocument(path, boost::json::value(-std::numeric_limits<double>::infinity())));

    BOOST_CHECK_EQUAL(contents(path), "first");
    BOOST_CHECK(!boost::filesystem::exists(sandbox.file("settings.json.tmp")));
}

/**
 * A document reaches the disk terminated, and leaves no sibling behind.
 **/
BOOST_AUTO_TEST_CASE(writer_write_document_test) {
    const Sandbox sandbox("document");
    const boost::filesystem::path path = sandbox.file("project.json");

    boost::json::object root;
    root["version"] = 1;
    BOOST_CHECK(v3d::asset::writeDocument(path, root));
    BOOST_CHECK_EQUAL(contents(path), "{ \"version\": 1 }\n");
    BOOST_CHECK(!boost::filesystem::exists(sandbox.file("project.json.tmp")));
}

/**
 * The document already there survives a write that does not complete. The rename is made to
 * fail by leaving a directory where the target is, which neither rename nor remove will
 * replace.
 **/
BOOST_AUTO_TEST_CASE(writer_failed_write_keeps_the_previous_document_test) {
    const Sandbox sandbox("failure");
    const boost::filesystem::path path = sandbox.file("settings.json");

    BOOST_REQUIRE(v3d::asset::writeFile(path, "first"));
    BOOST_CHECK_EQUAL(contents(path), "first");

    // a target that is a directory cannot be renamed onto
    const boost::filesystem::path blocked = sandbox.file("blocked.json");
    boost::filesystem::create_directories(blocked / "occupied");
    BOOST_CHECK(!v3d::asset::writeFile(blocked, "second"));
    BOOST_CHECK(boost::filesystem::is_directory(blocked));

    // and the temporary it wrote on the way is gone
    BOOST_CHECK(!boost::filesystem::exists(sandbox.file("blocked.json.tmp")));

    // the successful path still replaces what is there
    BOOST_REQUIRE(v3d::asset::writeFile(path, "third"));
    BOOST_CHECK_EQUAL(contents(path), "third");
}

/**
 * A directory that does not exist is a failure rather than one this creates: a path the caller
 * got wrong should not silently produce a tree of empty directories.
 **/
BOOST_AUTO_TEST_CASE(writer_missing_directory_test) {
    const Sandbox sandbox("missing");
    BOOST_CHECK(!v3d::asset::writeFile(sandbox.file("absent") / "file.json", "bytes"));
}
