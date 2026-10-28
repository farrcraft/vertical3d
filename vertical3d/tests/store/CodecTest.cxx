/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/BRep.h>
#include <api/brep/Primitives.h>
#include <api/curve/Curve.h>
#include <api/curve/Curves.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/Curve.h>
#include <vertical3d/src/scene/Group.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Light.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/store/Bytes.h>
#include <vertical3d/src/store/Codec.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
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
 * An item's blobs as a document's row would hold them.
 **/
v3d::editor::Blobs blobsOf(const v3d::editor::Item& item) {
    v3d::editor::Blobs blobs;
    for (const v3d::editor::Buffer& buffer : v3d::editor::codecOf(item.kind()).buffers(item)) {
        blobs[buffer.role] = v3d::editor::Blob{ .count = static_cast<std::int64_t>(buffer.count), .data = buffer.data };
    }
    return blobs;
}

/**
 * An item's payload as a document's row would hold it.
 **/
boost::json::object payloadOf(const v3d::editor::Item& item) {
    return boost::json::parse(boost::json::serialize(v3d::editor::codecOf(item.kind()).payload(item))).as_object();
}

/**
 * Write an item through its kind's codec and read it back, as a document's row would hold it.
 **/
boost::shared_ptr<v3d::editor::Item> roundTrip(const v3d::editor::Item& item, std::string* problem) {
    return v3d::editor::codecOf(item.kind()).read(payloadOf(item), blobsOf(item), problem);
}

/**
 * An item holding one curve of each type: an open poly curve, a closed Catmull-Rom curve, a
 * Bezier curve with a handle of each type, and a weighted NURBS curve with knots of its own. It
 * renders, at a width and resolution away from the defaults.
 **/
boost::shared_ptr<v3d::editor::Curve> everyCurve() {
    const boost::shared_ptr<v3d::curve::Curves> curves = boost::make_shared<v3d::curve::Curves>();
    curves->resolution(5);
    const std::vector<glm::vec3> points{ { 0, 0, 0 }, { 1, 2, 0 }, { 3, 1, 1 }, { 4, 4, 0 } };
    BOOST_REQUIRE(curves->add({ .shape = v3d::curve::Poly{ points } }).empty());
    BOOST_REQUIRE(curves->add({ .shape = v3d::curve::CatmullRom{ points }, .closed = true }).empty());

    v3d::curve::Bezier bezier;
    for (const glm::vec3& point : points) {
        bezier.points.push_back({ .position = point, .left = point - glm::vec3(0.5F, 0, 0), .right = point + glm::vec3(0, 0.5F, 0),
            .leftType = v3d::curve::Handle::Free, .rightType = v3d::curve::Handle::Free });
    }
    bezier.points[1].leftType = v3d::curve::Handle::Aligned;
    bezier.points[2].rightType = v3d::curve::Handle::Vector;
    bezier.points[3].leftType = v3d::curve::Handle::Automatic;
    BOOST_REQUIRE(curves->add({ .shape = bezier }).empty());

    v3d::curve::Nurbs nurbs;
    nurbs.order = 3;
    nurbs.points = points;
    nurbs.weights = { 1.0F, 0.5F, 2.0F, 1.0F };
    nurbs.knots = { 0, 0, 0, 1.5F, 4, 4, 4 };
    BOOST_REQUIRE(curves->add({ .shape = nurbs }).empty());

    const boost::shared_ptr<v3d::editor::Curve> item = boost::make_shared<v3d::editor::Curve>(curves);
    item->style({ .renders = true, .width = 0.125F });
    return item;
}

BOOST_AUTO_TEST_CASE(codec_every_kind_test) {
    // every kind has one codec, under a name no other has, and the name finds it again
    std::set<std::string_view> names;
    for (const v3d::editor::Item::Kind kind : { v3d::editor::Item::Kind::Mesh, v3d::editor::Item::Kind::Camera,
        v3d::editor::Item::Kind::Light, v3d::editor::Item::Kind::Group, v3d::editor::Item::Kind::Curve }) {
        const v3d::editor::Codec& codec = v3d::editor::codecOf(kind);
        BOOST_CHECK(codec.kind() == kind);
        BOOST_CHECK(names.insert(codec.name()).second);
        BOOST_CHECK(v3d::editor::codecNamed(codec.name()) == &codec);
        BOOST_CHECK_GT(codec.version(), 0);
    }
    BOOST_CHECK_EQUAL(v3d::editor::codecs().size(), names.size());
    BOOST_CHECK(v3d::editor::codecNamed("spline") == nullptr);

    // only a mesh and a curve write blobs
    BOOST_CHECK(v3d::editor::codecOf(v3d::editor::Item::Kind::Mesh).buffered());
    BOOST_CHECK(v3d::editor::codecOf(v3d::editor::Item::Kind::Curve).buffered());
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

BOOST_AUTO_TEST_CASE(codec_curve_round_trip_test) {
    // every curve comes back as it was written, with its type, closure, handles, weights and
    // knots, and so do the style and the resolution
    const boost::shared_ptr<v3d::editor::Curve> made = everyCurve();
    const v3d::editor::Curve& written = *made;
    std::string problem;
    const boost::shared_ptr<v3d::editor::Curve> read = v3d::editor::as<v3d::editor::Curve>(roundTrip(written, &problem));
    BOOST_REQUIRE_MESSAGE(read, problem);
    BOOST_CHECK(read->style() == written.style());
    BOOST_CHECK_EQUAL(read->geometry()->resolution(), 5U);
    BOOST_REQUIRE_EQUAL(read->geometry()->size(), written.geometry()->size());
    for (std::size_t index = 0; index < read->geometry()->size(); index++) {
        BOOST_CHECK_MESSAGE(read->geometry()->curve(index) == written.geometry()->curve(index), "curve " << index);
    }
}

BOOST_AUTO_TEST_CASE(codec_curve_refuses_test) {
    const boost::shared_ptr<v3d::editor::Curve> made = everyCurve();
    const v3d::editor::Curve& written = *made;
    const v3d::editor::Codec& codec = v3d::editor::codecOf(v3d::editor::Item::Kind::Curve);
    std::string problem;

    // a points blob one point short of what the curves hold
    v3d::editor::Blobs blobs = blobsOf(written);
    blobs["points"].count--;
    blobs["points"].data.resize(blobs["points"].data.size() - (3 * sizeof(float)));
    BOOST_CHECK(!codec.read(payloadOf(written), blobs, &problem));
    BOOST_CHECK(!problem.empty());

    // a handle type past the last one
    blobs = blobsOf(written);
    blobs["handles"].data[0] = std::byte{ 4 };
    problem.clear();
    BOOST_CHECK(!codec.read(payloadOf(written), blobs, &problem));
    BOOST_CHECK(!problem.empty());

    // knots that fall, which a curve cannot be evaluated with
    blobs = blobsOf(written);
    const float falling = -1.0F;
    std::memcpy(blobs["knots"].data.data() + (3 * sizeof(float)), &falling, sizeof(float));
    problem.clear();
    BOOST_CHECK(!codec.read(payloadOf(written), blobs, &problem));
    BOOST_CHECK(!problem.empty());

    // a width of nothing, and a resolution of nothing
    boost::json::object payload = payloadOf(written);
    payload["width"] = 0.0;
    BOOST_CHECK(!codec.read(payload, blobsOf(written), &problem));
    payload = payloadOf(written);
    payload["resolution"] = 0;
    BOOST_CHECK(!codec.read(payload, blobsOf(written), &problem));

    // a missing blob
    blobs = blobsOf(written);
    blobs.erase("weights");
    BOOST_CHECK(!codec.read(payloadOf(written), blobs, &problem));
}

};  // namespace
