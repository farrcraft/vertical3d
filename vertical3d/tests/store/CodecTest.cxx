/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/BRep.h>
#include <api/brep/Primitives.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/Group.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Light.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/store/Bytes.h>
#include <vertical3d/src/store/Codec.h>

#include <cstdint>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <boost/json.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/vec3.hpp>

namespace {

/**
 * Write an item through its kind's codec and read it back, as a document's row would hold it.
 **/
boost::shared_ptr<v3d::editor::Item> roundTrip(const v3d::editor::Item& item, std::string* problem) {
    const v3d::editor::Codec& codec = v3d::editor::codecOf(item.kind());
    v3d::editor::Blobs blobs;
    for (const v3d::editor::Buffer& buffer : codec.buffers(item)) {
        blobs[buffer.role] = v3d::editor::Blob{ .count = static_cast<std::int64_t>(buffer.count), .data = buffer.data };
    }
    const boost::json::value payload = boost::json::parse(boost::json::serialize(codec.payload(item)));
    return codec.read(payload.as_object(), blobs, problem);
}

BOOST_AUTO_TEST_CASE(codec_every_kind_test) {
    // every kind has one codec, under a name no other has, and the name finds it again
    std::set<std::string_view> names;
    for (const v3d::editor::Item::Kind kind : { v3d::editor::Item::Kind::Mesh, v3d::editor::Item::Kind::Camera,
        v3d::editor::Item::Kind::Light, v3d::editor::Item::Kind::Group }) {
        const v3d::editor::Codec& codec = v3d::editor::codecOf(kind);
        BOOST_CHECK(codec.kind() == kind);
        BOOST_CHECK(names.insert(codec.name()).second);
        BOOST_CHECK(v3d::editor::codecNamed(codec.name()) == &codec);
        BOOST_CHECK_GT(codec.version(), 0);
    }
    BOOST_CHECK_EQUAL(v3d::editor::codecs().size(), names.size());
    BOOST_CHECK(v3d::editor::codecNamed("spline") == nullptr);

    // only a mesh writes blobs
    BOOST_CHECK(v3d::editor::codecOf(v3d::editor::Item::Kind::Mesh).buffered());
    BOOST_CHECK(!v3d::editor::codecOf(v3d::editor::Item::Kind::Light).buffered());
}

BOOST_AUTO_TEST_CASE(codec_round_trip_test) {
    std::string problem;

    // each codec reads what it writes, and the item comes back as its kind
    v3d::editor::Light light;
    light.emission({ .type = v3d::editor::Light::Type::Spot, .colour = glm::vec3(0.5F, 0.25F, 1.0F), .intensity = 3.0F });
    const boost::shared_ptr<v3d::editor::Light> lit = v3d::editor::as<v3d::editor::Light>(roundTrip(light, &problem));
    BOOST_REQUIRE_MESSAGE(lit, problem);
    BOOST_CHECK(lit->emission().type == v3d::editor::Light::Type::Spot);
    BOOST_CHECK(lit->emission().colour == glm::vec3(0.5F, 0.25F, 1.0F));

    v3d::editor::Camera camera;
    camera.lens({ .projection = v3d::editor::Camera::Projection::Orthographic, .extent = 4.0F });
    const boost::shared_ptr<v3d::editor::Camera> seen = v3d::editor::as<v3d::editor::Camera>(roundTrip(camera, &problem));
    BOOST_REQUIRE_MESSAGE(seen, problem);
    BOOST_CHECK(seen->lens().projection == v3d::editor::Camera::Projection::Orthographic);
    BOOST_CHECK_EQUAL(seen->lens().extent, 4.0F);

    const v3d::editor::Mesh mesh(v3d::brep::cube());
    const boost::shared_ptr<v3d::editor::Mesh> shape = v3d::editor::as<v3d::editor::Mesh>(roundTrip(mesh, &problem));
    BOOST_REQUIRE_MESSAGE(shape, problem);
    BOOST_CHECK_EQUAL(shape->geometry()->faceCount(), mesh.geometry()->faceCount());
    BOOST_CHECK_EQUAL(shape->geometry()->edgeCount(), mesh.geometry()->edgeCount());

    BOOST_CHECK(v3d::editor::as<v3d::editor::Group>(roundTrip(v3d::editor::Group(), &problem)));
}

BOOST_AUTO_TEST_CASE(codec_refuses_test) {
    std::string problem;

    // a payload with a field its version does not define is refused, and so is a mesh missing
    // one of its blobs
    boost::json::object extra;
    extra["colour"] = 1.0;
    BOOST_CHECK(!v3d::editor::codecOf(v3d::editor::Item::Kind::Group).read(extra, {}, &problem));
    BOOST_CHECK(!problem.empty());

    problem.clear();
    BOOST_CHECK(!v3d::editor::codecOf(v3d::editor::Item::Kind::Mesh).read(boost::json::object(), {}, &problem));
    BOOST_CHECK(!problem.empty());
}

};  // namespace
