/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <string>

#include <boost/test/unit_test.hpp>

#include "../../src/view/CameraProfiles.h"
#include "../../../api/type/Camera.h"

#include <boost/make_shared.hpp>

namespace {

boost::shared_ptr<v3d::asset::Json> config(const std::string& text) {
    boost::json::value parsed = boost::json::parse(text);
    return boost::make_shared<v3d::asset::Json>("cameras", v3d::asset::Type::JsonDocument, parsed.as_object());
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
    v3d::editor::CameraProfiles profiles(logger());
    BOOST_REQUIRE(profiles.load(config(cameras)));

    BOOST_REQUIRE_EQUAL(profiles.names().size(), 2u);
    BOOST_CHECK_EQUAL(profiles.names()[0], "Top");
    BOOST_CHECK(profiles.has("Perspective"));
    BOOST_CHECK(!profiles.has("Nothing"));

    v3d::type::CameraProfile top = profiles.get("Top");
    BOOST_CHECK_EQUAL(top.orthographic(), true);
    BOOST_CHECK_CLOSE(top.orthoZoom(), 10.0f, 0.01f);
    BOOST_CHECK_CLOSE(top.clipping()[0], 0.1f, 0.01f);
    BOOST_CHECK_CLOSE(top.clipping()[1], 100.0f, 0.01f);
    BOOST_CHECK_EQUAL(top.adaptiveProjection(), true);
    BOOST_CHECK_EQUAL(top.adaptivePosition(), true);

    v3d::type::CameraProfile perspective = profiles.get("Perspective");
    BOOST_CHECK_EQUAL(perspective.orthographic(), false);
    BOOST_CHECK_CLOSE(perspective.fov(), 60.0f, 0.01f);
    BOOST_CHECK_EQUAL(perspective.adaptiveProjection(), false);
}

BOOST_AUTO_TEST_CASE(cameraprofiles_orientation_test) {
    // the lookat is what orients a profile: the three normals and the rotation have to
    // agree, and a table naming each of them separately is a table that can disagree
    v3d::editor::CameraProfiles profiles(logger());
    BOOST_REQUIRE(profiles.load(config(cameras)));

    v3d::type::CameraProfile top = profiles.get("Top");
    BOOST_CHECK_CLOSE(top.direction()[1], -1.0f, 0.01f);
    BOOST_CHECK_CLOSE(top.up()[2], 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(top.right()[0], 1.0f, 0.01f);

    // and the camera it makes sees the origin straight ahead, ten units along its own +z
    v3d::type::Camera camera(top);
    camera.createView();
    glm::vec4 origin = camera.view() * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    BOOST_CHECK_SMALL(origin[0], 0.001f);
    BOOST_CHECK_SMALL(origin[1], 0.001f);
    BOOST_CHECK_CLOSE(origin[2], 10.0f, 0.01f);
}

BOOST_AUTO_TEST_CASE(cameraprofiles_defaults_test) {
    // every field but the name has a default, so a sparse profile loads rather than failing
    v3d::editor::CameraProfiles profiles(logger());
    BOOST_REQUIRE(profiles.load(config("{\"cameras\": [{\"name\": \"Bare\"}]}")));

    v3d::type::CameraProfile bare = profiles.get("Bare");
    BOOST_CHECK_EQUAL(bare.name(), "Bare");
    BOOST_CHECK_EQUAL(bare.orthographic(), true);

    // a name that was never loaded comes back as a default profile rather than as nothing,
    // so a layout naming an unknown camera still has something to construct a view from
    BOOST_CHECK_EQUAL(profiles.get("Missing").name(), "Missing");
}

BOOST_AUTO_TEST_CASE(cameraprofiles_rejects_test) {
    v3d::editor::CameraProfiles profiles(logger());

    BOOST_CHECK(!profiles.load(config("{}")));
    BOOST_CHECK(!profiles.load(config("{\"cameras\": {}}")));
    // a profile with no name could never be looked up by a layout
    BOOST_CHECK(!profiles.load(config("{\"cameras\": [{\"orthographic\": true}]}")));
}
