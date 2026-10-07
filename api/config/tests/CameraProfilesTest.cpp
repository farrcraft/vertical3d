/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/config/CameraProfiles.h>
#include <api/log/Logger.h>
#include <api/type/camera/Camera.h>

#include <string>

#include <boost/test/unit_test.hpp>

#include <boost/json.hpp>
#include <boost/make_shared.hpp>

namespace {

boost::json::object config(const std::string& text) {
    boost::json::value parsed = boost::json::parse(text);
    return parsed.as_object();
}

boost::shared_ptr<v3d::log::Logger> logger() {
    return boost::make_shared<v3d::log::Logger>();
}

/**
 * Two of the profiles from data/cameras.json.
 **/
const char* const cameras =
"{\"cameras\": ["
"{\"name\": \"Top\", \"orthographic\": true, \"eye\": [0.0, 10.0, 0.0],"
" \"lookat\": [0.0, 0.0, 0.0], \"up\": [0.0, 0.0, 1.0], \"zoom\": 10.0,"
" \"aspect\": 1.33, \"near\": 0.1, \"far\": 100.0, \"adaptive\": \"both\"},"
"{\"name\": \"Perspective\", \"orthographic\": false, \"eye\": [0.0, 5.0, -20.0],"
" \"lookat\": [0.0, 0.0, 0.0], \"up\": [0.0, 1.0, 0.0], \"fov\": 60.0,"
" \"aspect\": 1.33, \"near\": 0.1, \"far\": 100.0, \"adaptive\": \"none\"}"
"]}";

};  // namespace

BOOST_AUTO_TEST_CASE(cameraprofiles_load_test) {
    v3d::config::CameraProfiles profiles(logger());
    BOOST_REQUIRE(profiles.load(config(cameras)));

    BOOST_REQUIRE_EQUAL(profiles.names().size(), 2u);
    BOOST_CHECK_EQUAL(profiles.names()[0], "Top");
    BOOST_CHECK(profiles.has("Perspective"));
    BOOST_CHECK(!profiles.has("Nothing"));

    v3d::type::camera::Profile top = profiles.get("Top");
    BOOST_CHECK_EQUAL(top.orthographic(), true);
    BOOST_CHECK_CLOSE(top.orthoZoom(), 10.0f, 0.01f);
    BOOST_CHECK_CLOSE(top.clipping()[0], 0.1f, 0.01f);
    BOOST_CHECK_CLOSE(top.clipping()[1], 100.0f, 0.01f);
    BOOST_CHECK_EQUAL(top.adaptiveProjection(), true);
    BOOST_CHECK_EQUAL(top.adaptivePosition(), true);

    v3d::type::camera::Profile perspective = profiles.get("Perspective");
    BOOST_CHECK_EQUAL(perspective.orthographic(), false);
    BOOST_CHECK_CLOSE(perspective.fov(), 60.0f, 0.01f);
    BOOST_CHECK_EQUAL(perspective.adaptiveProjection(), false);
}

BOOST_AUTO_TEST_CASE(cameraprofiles_orientation_test) {
    // lookat() orients a profile, so the three normals and the rotation agree; a table
    // naming each of them separately could disagree
    v3d::config::CameraProfiles profiles(logger());
    BOOST_REQUIRE(profiles.load(config(cameras)));

    v3d::type::camera::Profile top = profiles.get("Top");
    BOOST_CHECK_CLOSE(top.direction()[1], -1.0f, 0.01f);
    BOOST_CHECK_CLOSE(top.up()[2], 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(top.right()[0], 1.0f, 0.01f);

    // and the camera it makes sees the origin straight ahead, ten units along its own +z
    v3d::type::camera::Camera camera(top);
    camera.createView();
    glm::vec4 origin = camera.view() * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    BOOST_CHECK_SMALL(origin[0], 0.001f);
    BOOST_CHECK_SMALL(origin[1], 0.001f);
    BOOST_CHECK_CLOSE(origin[2], 10.0f, 0.01f);
}

BOOST_AUTO_TEST_CASE(cameraprofiles_defaults_test) {
    // every field but the name has a default, so a sparse profile loads rather than failing
    v3d::config::CameraProfiles profiles(logger());
    BOOST_REQUIRE(profiles.load(config("{\"cameras\": [{\"name\": \"Bare\"}]}")));

    v3d::type::camera::Profile bare = profiles.get("Bare");
    BOOST_CHECK_EQUAL(bare.name(), "Bare");
    BOOST_CHECK_EQUAL(bare.orthographic(), true);

    // a name that was never loaded comes back as a default profile rather than as nothing,
    // so a layout naming an unknown camera still has something to construct a view from
    BOOST_CHECK_EQUAL(profiles.get("Missing").name(), "Missing");
}

BOOST_AUTO_TEST_CASE(cameraprofiles_rejects_test) {
    v3d::config::CameraProfiles profiles(logger());

    BOOST_CHECK(!profiles.load(config("{}")));
    BOOST_CHECK(!profiles.load(config("{\"cameras\": {}}")));
    // a profile with no name could never be looked up by a layout
    BOOST_CHECK(!profiles.load(config("{\"cameras\": [{\"orthographic\": true}]}")));
    // a name or an adaptive setting that is not a string is refused rather than thrown
    BOOST_CHECK_NO_THROW(BOOST_CHECK(!profiles.load(config("{\"cameras\": [{\"name\": 5}]}"))));
    BOOST_CHECK_NO_THROW(BOOST_CHECK(!profiles.load(config("{\"cameras\": [{\"name\": \"a\", \"adaptive\": 3}]}"))));
}

/**
 * A vector holding something other than numbers is the wrong shape, and the default is kept.
 **/
BOOST_AUTO_TEST_CASE(cameraprofiles_vector_of_strings_test) {
    v3d::config::CameraProfiles profiles(logger());

    BOOST_CHECK_NO_THROW(BOOST_REQUIRE(profiles.load(config("{\"cameras\": [{\"name\": \"a\", \"eye\": [\"x\", 1, 2]}]}"))));
    BOOST_CHECK(profiles.has("a"));

    const glm::vec3 eye = profiles.get("a").eye();
    BOOST_CHECK_SMALL(eye[0], 0.001f);
    BOOST_CHECK_SMALL(eye[1], 0.001f);
    BOOST_CHECK_CLOSE(eye[2], -10.0f, 0.01f);
}
