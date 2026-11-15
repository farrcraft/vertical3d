/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/integrate/Profile.h>
#include <moya/libmoya/integrate/Render.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <set>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "Tracing.h"

namespace v3d::moya::tracing {
namespace {

namespace film = v3d::moya::film;
namespace integrate = v3d::moya::integrate;
namespace scene = v3d::moya::scene;

/** The picture's side, in pixels. **/
constexpr std::int32_t kSide = 16;

/** A pixel inside the square, a pixel its edge crosses, and a corner it does not reach. **/
constexpr std::uint32_t kInside = 8;
constexpr std::uint32_t kEdge = 12;
constexpr std::uint32_t kCorner = 0;

/** How far a depth is from the square's five units, as a share of it, by float rounding alone. **/
constexpr float kRounding = 1.0e-6F;

/** The value of the square's constant primvar, mask. **/
constexpr float kMask = 0.75F;

/**
 * A square of half-side one half in the plane z = 0, facing a camera five units back along -z,
 * under a dome. The camera keeps the orthographic projection the scene falls back to, whose
 * screen runs from -1 to 1. The square covers the picture's middle half on each axis, and its
 * edge crosses the pixels of the thirteenth column. The square's st runs from zero at
 * its bottom left to one at its top right, and its primvar mask is kMask throughout. The camera's
 * shutter is open from zero to one, and the square moves along x by its drift over it.
 **/
struct Squared {
    scene::Scene made { scene::kinds() };
    std::vector<scene::Handle> channels;

    explicit Squared(float drift = 0.0F) {
        const scene::Handle options = made.create("options", "options");
        const scene::Handle camera = made.create("camera", "camera");
        made.set(options, "camera", scene::Value::handle(camera));
        made.set(options, "resolution", scene::Value::integers({ kSide, kSide }));
        made.set(options, "samples", scene::Value::integer(16));
        made.set(camera, "transform", scene::Value::matrix(moved(0.0F, 0.0F, -5.0F)));
        made.set(camera, "shutter", scene::Value::floats({ 0.0F, 1.0F }));
        made.create("domeLight", "sky");

        const scene::Handle mesh = made.create("mesh", "square");
        made.set(mesh, "P", scene::Value::points({ { -0.5F, -0.5F, 0.0F }, { 0.5F, -0.5F, 0.0F }, { 0.5F, 0.5F, 0.0F }, { -0.5F, 0.5F, 0.0F } }));
        made.set(mesh, "vertices", scene::Value::integers({ 3, 3 }));
        made.set(mesh, "indices", scene::Value::integers({ 0, 2, 1, 0, 3, 2 }));
        const scene::Value st = scene::Value::floats({ 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 1.0F, 0.0F, 1.0F });
        made.declare(mesh,
            scene::Declaration { .name = "st", .type = st.type(), .count = scene::Declaration::kAny, .width = 2, .fallback = st, .interpolation = scene::Interpolation::Vertex });
        made.set(mesh, "st", st);
        const scene::Value mask = scene::Value::floats({ kMask });
        made.declare(mesh,
            scene::Declaration { .name = "mask", .type = mask.type(), .count = scene::Declaration::kAny, .fallback = mask, .interpolation = scene::Interpolation::Constant });
        made.set(mesh, "mask", mask);
        instanced(made, "placed", mesh, {}, { 0.0F, 1.0F }, { glm::mat4(1.0F), moved(drift, 0.0F, 0.0F) });
    }

    /** A channel of what it collects, combined by its filter. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the name, then what it collects
    void channel(const std::string & name, const std::string & type, const std::string & source, std::int32_t components, const std::string & filter = "default") {
        const scene::Handle object = made.create("channel", name);
        made.set(object, "sourceType", scene::Value::string(type));
        made.set(object, "sourceName", scene::Value::string(source));
        made.set(object, "components", scene::Value::integer(components));
        made.set(object, "filter", scene::Value::string(filter));
        channels.push_back(object);
    }

    /** The picture of one output that holds every channel, filling a profile when given one. **/
    film::Snapshot render(integrate::Profile * profile = nullptr) {
        const scene::Handle output = made.create("output", "picture");
        made.set(output, "file", scene::Value::string("picture.exr"));
        made.set(output, "channels", scene::Value::handles(channels));
        made.commit();
        made.apply();
        v3d::moya::device::CpuDevice device;
        return integrate::render(device, made, {}, colour::Config(), {}, profile);
    }
};

/** A pixel's float in a channel. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the pixel, then the component
float at(const film::Snapshot & picture, const std::string & name, std::uint32_t x, std::uint32_t y, std::uint32_t component = 0) {
    return picture.at(x, y, picture.offset(name) + component);
}

// every camera ray that meets the square is five units deep, to float rounding, and one that
// meets nothing is infinitely deep. The depth takes the closest sample, so a pixel the square's
// edge crosses is as deep as the square
BOOST_AUTO_TEST_CASE(moya_firsthit_a_square_facing_the_camera_is_five_units_deep_test) {
    Squared squared;
    squared.channel("depth", "raw", "depth", 1);
    const film::Snapshot picture = squared.render();
    for (std::uint32_t y = 0; y < kSide; ++y) {
        for (std::uint32_t x = 0; x < kSide; ++x) {
            const bool inside = x >= 4U && x < 12U && y >= 4U && y < 12U;
            if (inside) {
                BOOST_TEST_CONTEXT("pixel " << x << ", " << y) {
                    BOOST_TEST(at(picture, "depth", x, y) == 5.0F, boost::test_tools::tolerance(kRounding));
                }
            }
        }
    }
    BOOST_TEST(at(picture, "depth", kEdge, kInside) == 5.0F, boost::test_tools::tolerance(kRounding));
    BOOST_TEST(at(picture, "depth", kCorner, kCorner) == std::numeric_limits<float>::infinity());
}

// a counted render runs another entry point, which fills the channels at the first hit as the
// render's own does, so each pixel's depth is the same
BOOST_AUTO_TEST_CASE(moya_firsthit_a_counted_render_fills_the_same_channels_test) {
    Squared plain;
    plain.channel("depth", "raw", "depth", 1);
    const film::Snapshot want = plain.render();
    Squared counted;
    counted.channel("depth", "raw", "depth", 1);
    integrate::Profile profile { .counted = true };
    const film::Snapshot got = counted.render(&profile);
    for (std::uint32_t y = 0; y < kSide; ++y) {
        for (std::uint32_t x = 0; x < kSide; ++x) {
            BOOST_TEST_CONTEXT("pixel " << x << ", " << y) {
                BOOST_TEST(at(got, "depth", x, y) == at(want, "depth", x, y));
            }
        }
    }
}

// a filter of min or max keeps the least or the greatest of a pixel's samples, so the pixel the
// square's edge crosses is five deep at its least and infinitely deep at its most
BOOST_AUTO_TEST_CASE(moya_firsthit_a_filter_names_how_samples_combine_test) {
    Squared squared;
    squared.channel("least", "raw", "depth", 1, "min");
    squared.channel("most", "raw", "depth", 1, "max");
    const film::Snapshot picture = squared.render();
    BOOST_TEST(at(picture, "least", kEdge, kInside) == 5.0F, boost::test_tools::tolerance(kRounding));
    BOOST_TEST(at(picture, "most", kEdge, kInside) == std::numeric_limits<float>::infinity());
    BOOST_TEST(at(picture, "most", kInside, kInside) == 5.0F, boost::test_tools::tolerance(kRounding));
}

// the position is on the square, within the pixel; the normal is the square's; and the face is
// one of the square's two triangles, both of which some pixel shows. A pixel the square misses
// has a zero normal and no instance or face
BOOST_AUTO_TEST_CASE(moya_firsthit_position_normal_and_ids_are_the_squares_test) {
    Squared squared;
    squared.channel("position", "raw", "position", 3);
    squared.channel("normal", "raw", "normal", 3);
    squared.channel("instance", "raw", "instance", 1);
    squared.channel("face", "raw", "face", 1);
    const film::Snapshot picture = squared.render();
    // a pixel is an eighth of a unit wide, and its centre is at a sixteenth past its left edge
    const float pixel = 2.0F / static_cast<float>(kSide);
    const float centre = -1.0F + ((static_cast<float>(kInside) + 0.5F) * pixel);
    BOOST_TEST(std::fabs(at(picture, "position", kInside, kInside, 0) - centre) <= pixel);
    BOOST_TEST(at(picture, "position", kInside, kInside, 2) == 0.0F, boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(std::fabs(at(picture, "normal", kInside, kInside, 2)) == 1.0F, boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(at(picture, "instance", kInside, kInside) >= 0.0F);
    BOOST_TEST(at(picture, "normal", kCorner, kCorner, 2) == 0.0F);
    BOOST_TEST(at(picture, "instance", kCorner, kCorner) == -1.0F);
    BOOST_TEST(at(picture, "face", kCorner, kCorner) == -1.0F);
    std::set<float> faces;
    for (std::uint32_t y = 4; y < 12U; ++y) {
        for (std::uint32_t x = 4; x < 12U; ++x) {
            faces.insert(at(picture, "face", x, y));
        }
    }
    BOOST_TEST((faces == std::set<float> { 0.0F, 1.0F }));
}

// st is the filtered mean of the square's texture coordinates, which grow by an eighth of the
// square's side, a quarter of the picture's, across each pixel
BOOST_AUTO_TEST_CASE(moya_firsthit_st_follows_the_squares_texture_coordinates_test) {
    Squared squared;
    squared.channel("st", "raw", "st", 2);
    const film::Snapshot picture = squared.render();
    const float left = at(picture, "st", 5, kInside, 0);
    const float right = at(picture, "st", 10, kInside, 0);
    // st at a pixel's centre: five pixels of the square's eight from its left edge, and ten
    BOOST_TEST(right - left == 5.0F / 8.0F, boost::test_tools::tolerance(0.02F));
    BOOST_TEST(left == 1.5F / 8.0F, boost::test_tools::tolerance(0.1F));
}

// a primvar channel reads the square's constant mask, which no material reads. A pixel the
// square misses holds zero
BOOST_AUTO_TEST_CASE(moya_firsthit_a_primvar_of_a_constant_reads_back_the_constant_test) {
    Squared squared;
    squared.channel("mask", "primvar", "mask", 1);
    const film::Snapshot picture = squared.render();
    BOOST_TEST(at(picture, "mask", kInside, kInside) == kMask, boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(at(picture, "mask", kCorner, kCorner) == 0.0F);
}

// the square moves a quarter of a unit along x while the shutter is open, which is two of the
// picture's pixels, so every pixel inside it moves two pixels right. One that holds still does not
// move, and nor does a pixel that sees only the dome
BOOST_AUTO_TEST_CASE(moya_firsthit_a_moving_square_moves_across_the_picture_test) {
    Squared moving(0.25F);
    moving.channel("motion", "raw", "motion", 2);
    const film::Snapshot picture = moving.render();
    BOOST_TEST(at(picture, "motion", kInside, kInside, 0) == 2.0F, boost::test_tools::tolerance(1.0e-4F));
    BOOST_TEST(at(picture, "motion", kInside, kInside, 1) == 0.0F, boost::test_tools::tolerance(1.0e-4F));
    BOOST_TEST(at(picture, "motion", kCorner, kCorner, 0) == 0.0F);

    Squared still;
    still.channel("motion", "raw", "motion", 2);
    BOOST_TEST(at(still.render(), "motion", kInside, kInside, 0) == 0.0F, boost::test_tools::tolerance(1.0e-4F));
}

// a mesh whose own points rise a quarter of a unit while the shutter is open moves two pixels up
// the picture, which is minus two in raster y
BOOST_AUTO_TEST_CASE(moya_firsthit_a_deforming_mesh_moves_as_its_points_do_test) {
    Squared squared;
    const std::vector<glm::vec3> low { { -0.5F, -0.5F, 0.0F }, { 0.5F, -0.5F, 0.0F }, { 0.5F, 0.5F, 0.0F }, { -0.5F, -0.5F, 0.0F }, { 0.5F, 0.5F, 0.0F },
        { -0.5F, 0.5F, 0.0F } };
    std::vector<glm::vec3> high = low;
    for (glm::vec3 & point : high) {
        point.y += 0.25F;
    }
    // a second square, in front of the first, that the camera sees in its place
    std::vector<glm::vec3> nearer = low;
    std::vector<glm::vec3> nearerHigh = high;
    for (std::size_t i = 0; i < nearer.size(); ++i) {
        nearer[i].z = -1.0F;
        nearerHigh[i].z = -1.0F;
    }
    instanced(squared.made, "rising", mesh(squared.made, "rises", { 0.0F, 1.0F }, { nearer, nearerHigh }), {}, glm::mat4(1.0F));
    squared.channel("motion", "raw", "motion", 2);
    const film::Snapshot picture = squared.render();
    BOOST_TEST(at(picture, "motion", kInside, kInside, 0) == 0.0F, boost::test_tools::tolerance(1.0e-4F));
    BOOST_TEST(at(picture, "motion", kInside, kInside, 1) == -2.0F, boost::test_tools::tolerance(1.0e-4F));
}

// a raw value the path tracer does not compute, a value of other components than its channel's,
// and a light path expression each hold zero
BOOST_AUTO_TEST_CASE(moya_firsthit_a_channel_it_cannot_fill_holds_zero_test) {
    Squared squared;
    squared.channel("unknown", "raw", "curvature", 1);
    squared.channel("narrow", "raw", "normal", 1);
    squared.channel("light", "lpe", "C<RD>L", 3);
    const film::Snapshot picture = squared.render();
    for (const char * name : { "unknown", "narrow", "light" }) {
        BOOST_TEST(at(picture, name, kInside, kInside) == 0.0F, name);
    }
}

};  // namespace
};  // namespace v3d::moya::tracing
