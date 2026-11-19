/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/kernel/light/Emission.h>
#include <moya/libmoya/kernel/light/Light.h>
#include <moya/libmoya/kernel/light/LightSample.h>
#include <moya/libmoya/kernel/light/Sampling.h>
#include <moya/libmoya/kernel/light/spherical/SphericalTriangle.h>
#include <moya/libmoya/kernel/surface/Frame.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "Tracing.h"

namespace v3d::moya::tracing {
namespace {

namespace light = v3d::moya::kernel::light;
namespace surface = v3d::moya::kernel::surface;

constexpr float kTurn = 6.28318530717958647692F;

/** The bins along each of a shape's two parameters, and the points summed along a bin's side. **/
constexpr std::size_t kBins = 8;
constexpr std::size_t kSteps = 16;

/** How many points each case draws. **/
constexpr std::size_t kDraws = 200000;

/** What a case found: the expected shares' sum, the chi-squared statistic, and its bins. **/
struct Fit {
    double total { 0.0 };
    double chiSquared { 0.0 };
    std::size_t bins { 0 };
    std::size_t outside { 0 };
    std::size_t emptyBinsHit { 0 };
};

light::Light shaped(light::Shape shape, const glm::vec2 & size) {
    light::Light out;
    out.shape = shape;
    out.size = size;
    return out;
}

/**
 * The parameters in the unit square of a point of a light's shape in its prototype's space: the
 * ones light::areaPoint() maps onto it.
 **/
glm::vec2 parametersOf(const light::Light & shape, const glm::vec3 & local) {
    const float around = std::atan2(local.y, local.x);
    const float turn = (around < 0.0F ? around + kTurn : around) / kTurn;
    switch (shape.shape) {
        case light::Shape::Rect:
            return glm::vec2 { (local.x / shape.size.x) + 0.5F, (local.y / shape.size.y) + 0.5F };
        case light::Shape::Disk:
            return glm::vec2 { glm::dot(glm::vec2(local), glm::vec2(local)) / (shape.size.x * shape.size.x), turn };
        case light::Shape::Sphere:
            return glm::vec2 { 0.5F * (1.0F - (local.z / shape.size.x)), turn };
        case light::Shape::Cylinder:
            return glm::vec2 { (local.z / shape.size.y) + 0.5F, turn };
        case light::Shape::Point:
        case light::Shape::Distant:
        case light::Shape::Mesh:
        case light::Shape::Dome:
        case light::Shape::Volume:
            break;
    }
    return glm::vec2 { 0.0F };
}

/** The bin of a pair of parameters, or none outside the unit square. **/
std::size_t binOf(const glm::vec2 & parameters) {
    const auto along = [](float value) {
        // a value just below zero belongs to the first bin
        const auto at = static_cast<std::size_t>(std::max(0.0F, std::floor(value * static_cast<float>(kBins))));
        return at < kBins ? at : kBins - 1;
    };
    // a parameter that is not a number lies outside too
    const bool inside = parameters.x >= -1.0e-4F && parameters.x <= 1.0001F && parameters.y >= -1.0e-4F && parameters.y <= 1.0001F;
    if (!inside) {
        return kBins * kBins;
    }
    return (along(parameters.y) * kBins) + along(parameters.x);
}

/**
 * Draws points of a light from a point and bins them over its shape's parameters. Each bin's
 * expected share is light::density() over it, times the cosine over the distance squared, summed
 * over the bin's area in the world.
 **/
Fit fitted(const light::Light & shape, const glm::mat4 & toWorld, const glm::vec3 & from) {
    std::vector<double> expected(kBins * kBins, 0.0);
    const glm::mat3 cofactor = light::cofactor(glm::mat3(toWorld));
    const double cell = static_cast<double>(light::localArea(shape)) / static_cast<double>(kBins * kBins * kSteps * kSteps);
    for (std::size_t row = 0; row < kBins * kSteps; ++row) {
        for (std::size_t column = 0; column < kBins * kSteps; ++column) {
            const glm::vec2 u { (static_cast<float>(column) + 0.5F) / static_cast<float>(kBins * kSteps),
                (static_cast<float>(row) + 0.5F) / static_cast<float>(kBins * kSteps) };
            const glm::vec3 local = light::areaPoint(shape, u);
            const glm::vec3 point { toWorld * glm::vec4(local, 1.0F) };
            const glm::vec3 normal = light::emittingNormal(shape, local);
            const glm::vec3 turned = cofactor * glm::normalize(normal);
            const float stretch = glm::length(turned);
            const glm::vec3 toPoint = point - from;
            const float squared = glm::dot(toPoint, toPoint);
            const float cosine = std::fabs(glm::dot(turned / stretch, toPoint)) / std::sqrt(squared);
            const double share = static_cast<double>(light::density(shape, toWorld, from, point)) * cosine / squared * stretch * cell;
            expected[((row / kSteps) * kBins) + (column / kSteps)] += share;
        }
    }
    std::vector<double> observed(kBins * kBins, 0.0);
    Fit out;
    std::uint32_t seed = 7;
    for (std::size_t draw = 0; draw < kDraws; ++draw) {
        const glm::vec2 u { next(seed), next(seed) };
        const light::LightSample drawn = light::sample(shape, toWorld, from, u);
        if (!drawn.found()) {
            out.outside++;
            continue;
        }
        const glm::vec3 local { glm::inverse(toWorld) * glm::vec4(drawn.point, 1.0F) };
        const std::size_t bin = binOf(parametersOf(shape, local));
        if (bin >= observed.size()) {
            out.outside++;
            continue;
        }
        observed[bin] += 1.0;
    }
    for (std::size_t bin = 0; bin < expected.size(); ++bin) {
        out.total += expected[bin];
        const double count = expected[bin] * static_cast<double>(kDraws);
        if (count < 1.0e-6) {
            out.emptyBinsHit += observed[bin] > 0.0 ? 1U : 0U;
            continue;
        }
        if (count >= 5.0) {
            out.chiSquared += (observed[bin] - count) * (observed[bin] - count) / count;
            out.bins++;
        }
    }
    return out;
}

/** Whether a fit holds: shares that sum to one, and counts that follow them. **/
void holds(const Fit & fit, const std::string & label) {
    BOOST_TEST_CONTEXT(label) {
        BOOST_TEST(std::fabs(fit.total - 1.0) < 0.01);
        BOOST_TEST(fit.outside == 0U);
        BOOST_TEST(fit.emptyBinsHit == 0U);
        BOOST_TEST_REQUIRE(fit.bins > 8U);
        const double freedom = static_cast<double>(fit.bins - 1);
        BOOST_TEST(fit.chiSquared < freedom + (4.0 * std::sqrt(2.0 * freedom)));
    }
}

/** A turn and a move that put a light at a slant, away from the origin. **/
glm::mat4 slanted() {
    return glm::rotate(moved(0.3F, -0.2F, 0.1F), glm::radians(30.0F), glm::normalize(glm::vec3 { 1.0F, 1.0F, 0.0F }));
}

/** A point in a light's prototype's space, placed in the world. **/
glm::vec3 placed(const glm::mat4 & toWorld, const glm::vec3 & local) {
    return glm::vec3 { toWorld * glm::vec4(local, 1.0F) };
}

// a rect at right angles is sampled as a spherical rectangle, and a sheared one, or one too small
// to see, by its area
BOOST_AUTO_TEST_CASE(moya_lightsampling_a_rect_light_samples_follow_their_density_test) {
    const light::Light rect = shaped(light::Shape::Rect, glm::vec2 { 2.0F, 1.0F });
    const glm::mat4 place = slanted();
    BOOST_TEST((light::methodOf(rect, place, placed(place, { 0.4F, 0.2F, -0.8F })) == light::Method::Rectangle));
    holds(fitted(rect, place, placed(place, { 0.4F, 0.2F, -0.8F })), "near");
    holds(fitted(rect, place, placed(place, { 1.5F, 0.9F, -0.2F })), "grazing");
    glm::mat4 sheared = place;
    sheared[1] += sheared[0] * 0.5F;
    BOOST_TEST((light::methodOf(rect, sheared, placed(sheared, { 0.0F, 0.0F, -1.0F })) == light::Method::Area));
    holds(fitted(rect, sheared, placed(sheared, { 0.0F, 0.0F, -1.0F })), "sheared");
    const light::Light small = shaped(light::Shape::Rect, glm::vec2 { 0.1F, 0.1F });
    BOOST_TEST(light::rectangleOf(small, place, placed(place, { 0.0F, 0.0F, -5.0F })).solidAngle < light::kSmallestSolidAngle);
    holds(fitted(small, place, placed(place, { 0.0F, 0.0F, -5.0F })), "small");
}

// a disk that stays round is sampled as a spherical ellipse, from a slant and along its normal,
// and one stretched into an ellipse by its area
BOOST_AUTO_TEST_CASE(moya_lightsampling_a_disk_light_samples_follow_their_density_test) {
    const light::Light disk = shaped(light::Shape::Disk, glm::vec2 { 1.0F, 0.0F });
    const glm::mat4 place = slanted();
    BOOST_TEST((light::methodOf(disk, place, placed(place, { 0.8F, 0.3F, -0.6F })) == light::Method::Ellipse));
    holds(fitted(disk, place, placed(place, { 0.8F, 0.3F, -0.6F })), "slanted");
    holds(fitted(disk, place, placed(place, { 0.0F, 0.0F, -2.0F })), "along its normal");
    holds(fitted(disk, place, placed(place, { 2.5F, -1.0F, -0.15F })), "grazing");
    const glm::mat4 stretched = glm::scale(place, glm::vec3 { 2.0F, 1.0F, 1.0F });
    BOOST_TEST((light::methodOf(disk, stretched, placed(stretched, { 0.0F, 0.0F, -1.0F })) == light::Method::Area));
    holds(fitted(disk, stretched, placed(stretched, { 0.0F, 0.0F, -1.0F })), "stretched");
}

// a sphere that stays round is sampled in the cone it subtends, a narrow one by its expansion,
// and a stretched one by its area
BOOST_AUTO_TEST_CASE(moya_lightsampling_a_sphere_light_samples_follow_their_density_test) {
    const light::Light sphere = shaped(light::Shape::Sphere, glm::vec2 { 1.0F, 0.0F });
    const glm::mat4 place = slanted();
    BOOST_TEST((light::methodOf(sphere, place, placed(place, { 0.5F, 0.0F, -3.0F })) == light::Method::Cone));
    holds(fitted(sphere, place, placed(place, { 0.5F, 0.0F, -3.0F })), "near");
    holds(fitted(sphere, place, placed(place, { 0.0F, 1.2F, 0.0F })), "close");
    const light::Light narrow = shaped(light::Shape::Sphere, glm::vec2 { 0.001F, 0.0F });
    holds(fitted(narrow, place, placed(place, { 0.0F, 0.0F, -1.0F })), "narrow");
    const glm::mat4 stretched = glm::scale(place, glm::vec3 { 1.0F, 1.0F, 2.0F });
    BOOST_TEST((light::methodOf(sphere, stretched, placed(stretched, { 0.0F, 0.0F, -3.0F })) == light::Method::Area));
    holds(fitted(sphere, stretched, placed(stretched, { 0.0F, 0.0F, -3.0F })), "stretched");
}

// a cylinder is sampled by its area
BOOST_AUTO_TEST_CASE(moya_lightsampling_a_cylinder_light_samples_follow_their_density_test) {
    const light::Light cylinder = shaped(light::Shape::Cylinder, glm::vec2 { 0.5F, 2.0F });
    const glm::mat4 place = slanted();
    holds(fitted(cylinder, place, placed(place, { 1.5F, 0.5F, 0.3F })), "beside");
}

/**
 * Draws directions to a distant light with an angle and bins them by their cosine to the light's
 * direction across its cone, and by their azimuth. Each bin's expected share is light::density()
 * over it, summed over its solid angle.
 **/
Fit fittedCone(const light::Light & distant, const glm::mat4 & toWorld) {
    const glm::vec3 from { 0.0F };
    const glm::vec3 towards = -light::axisOf(distant, toWorld);
    const surface::Frame frame = surface::Frame::around(towards);
    const float rise = light::distantRise(distant);
    const auto direction = [&](const glm::vec2 & u) {
        const float drop = u.x * rise;
        const float cosine = 1.0F - drop;
        const float sine = std::sqrt(std::fmax(0.0F, drop * (2.0F - drop)));
        return frame.toWorld(glm::vec3 { sine * std::cos(kTurn * u.y), sine * std::sin(kTurn * u.y), cosine });
    };
    std::vector<double> expected(kBins * kBins, 0.0);
    const double cell = static_cast<double>(rise) * kTurn / static_cast<double>(kBins * kBins * kSteps * kSteps);
    for (std::size_t row = 0; row < kBins * kSteps; ++row) {
        for (std::size_t column = 0; column < kBins * kSteps; ++column) {
            const glm::vec2 u { (static_cast<float>(column) + 0.5F) / static_cast<float>(kBins * kSteps),
                (static_cast<float>(row) + 0.5F) / static_cast<float>(kBins * kSteps) };
            const double share = static_cast<double>(light::density(distant, toWorld, from, from + direction(u))) * cell;
            expected[((row / kSteps) * kBins) + (column / kSteps)] += share;
        }
    }
    std::vector<double> observed(kBins * kBins, 0.0);
    Fit out;
    std::uint32_t seed = 7;
    for (std::size_t draw = 0; draw < kDraws; ++draw) {
        const light::LightSample drawn = light::sample(distant, toWorld, from, glm::vec2 { next(seed), next(seed) });
        const glm::vec3 local = frame.toLocal(drawn.incoming);
        const float around = std::atan2(local.y, local.x);
        // one less the cosine as the sine squared over one plus the cosine, which stays precise
        const float drop = ((local.x * local.x) + (local.y * local.y)) / (1.0F + local.z);
        const std::size_t bin = binOf(glm::vec2 { drop / rise, (around < 0.0F ? around + kTurn : around) / kTurn });
        if (!drawn.found() || bin >= observed.size() || !(glm::distance(drawn.point, from + drawn.incoming) < 1.0e-5F)) {
            out.outside++;
            continue;
        }
        observed[bin] += 1.0;
    }
    for (std::size_t bin = 0; bin < expected.size(); ++bin) {
        out.total += expected[bin];
        const double count = expected[bin] * static_cast<double>(kDraws);
        if (count >= 5.0) {
            out.chiSquared += (observed[bin] - count) * (observed[bin] - count) / count;
            out.bins++;
        }
    }
    return out;
}

// a distant light with an angle is sampled uniformly in its cone, whether as wide as the sun or
// wider than a quarter turn, and its density is zero outside it
BOOST_AUTO_TEST_CASE(moya_lightsampling_a_distant_light_samples_follow_their_density_test) {
    const glm::mat4 place = slanted();
    for (const float degrees : { 0.53F, 20.0F, 200.0F }) {
        const light::Light distant = shaped(light::Shape::Distant, glm::vec2 { 0.5F * glm::radians(degrees), 0.0F });
        holds(fittedCone(distant, place), "angle " + std::to_string(degrees));
    }
    const light::Light narrow = shaped(light::Shape::Distant, glm::vec2 { glm::radians(10.0F), 0.0F });
    const glm::vec3 towards = -light::axisOf(narrow, place);
    BOOST_TEST(light::density(narrow, place, glm::vec3 { 0.0F }, towards) > 0.0F);
    BOOST_TEST(light::density(narrow, place, glm::vec3 { 0.0F }, surface::Frame::around(towards).toWorld(glm::vec3 { 0.2F, 0.0F, 0.98F })) == 0.0F);
}

// a point is drawn where it stands, and a distant light of no angle along its direction, each with
// a density of one. No direction a BSDF draws meets either, so neither has a density for one
BOOST_AUTO_TEST_CASE(moya_lightsampling_a_point_and_a_parallel_light_are_drawn_whole_test) {
    const glm::mat4 place = slanted();
    const glm::vec3 from { 2.0F, 1.0F, -1.0F };
    const light::LightSample point = light::sample(shaped(light::Shape::Point, glm::vec2 { 0.0F }), place, from, glm::vec2 { 0.3F, 0.6F });
    BOOST_TEST(point.density == 1.0F);
    BOOST_TEST(glm::distance(point.point, glm::vec3(place[3])) < 1.0e-6F);
    BOOST_TEST(glm::distance(point.incoming, glm::normalize(glm::vec3(place[3]) - from)) < 1.0e-6F);
    BOOST_TEST(light::density(shaped(light::Shape::Point, glm::vec2 { 0.0F }), place, from, point.point) == 0.0F);
    const light::Light parallel = shaped(light::Shape::Distant, glm::vec2 { 0.0F });
    const light::LightSample along = light::sample(parallel, place, from, glm::vec2 { 0.3F, 0.6F });
    BOOST_TEST(along.density == 1.0F);
    // it shines along its -z, so it is met looking up its +z
    BOOST_TEST(glm::distance(along.incoming, glm::normalize(glm::vec3(place[2]))) < 1.0e-6F);
    BOOST_TEST(light::density(parallel, place, from, from + along.incoming) == 0.0F);
}

/**
 * Draws points of a triangle from a point and bins them by their parameters: one less the first
 * corner's weight, and the third's share of what the other two hold. Each bin's expected share is
 * light::spherical::triangleDensity() over it, times the cosine over the distance squared, summed over the
 * bin's area, which is twice the triangle's area times the first parameter.
 **/
Fit fittedTriangle(const glm::vec3 (&corners)[3], const glm::vec3 & from) {
    const glm::vec3 normal = glm::normalize(glm::cross(corners[1] - corners[0], corners[2] - corners[0]));
    const double twice = glm::length(glm::cross(corners[1] - corners[0], corners[2] - corners[0]));
    const double cell = 1.0 / static_cast<double>(kBins * kBins * kSteps * kSteps);
    std::vector<double> expected(kBins * kBins, 0.0);
    for (std::size_t row = 0; row < kBins * kSteps; ++row) {
        for (std::size_t column = 0; column < kBins * kSteps; ++column) {
            const float out = (static_cast<float>(column) + 0.5F) / static_cast<float>(kBins * kSteps);
            const float across = (static_cast<float>(row) + 0.5F) / static_cast<float>(kBins * kSteps);
            const glm::vec3 point = corners[0] + (((corners[1] - corners[0]) * (1.0F - across)) + ((corners[2] - corners[0]) * across)) * out;
            const glm::vec3 toPoint = point - from;
            const float squared = glm::dot(toPoint, toPoint);
            const float cosine = std::fabs(glm::dot(normal, toPoint)) / std::sqrt(squared);
            const double share = static_cast<double>(light::spherical::triangleDensity(corners, normal, from, point)) * cosine / squared * twice * out * cell;
            expected[((row / kSteps) * kBins) + (column / kSteps)] += share;
        }
    }
    std::vector<double> observed(kBins * kBins, 0.0);
    Fit out;
    std::uint32_t seed = 7;
    for (std::size_t draw = 0; draw < kDraws; ++draw) {
        const light::LightSample drawn = light::spherical::triangleSample(corners, normal, from, glm::vec2 { next(seed), next(seed) });
        // the point's weights, from its place against two of the triangle's edges
        const glm::vec3 first = corners[1] - corners[0];
        const glm::vec3 second = corners[2] - corners[0];
        const glm::vec3 at = drawn.point - corners[0];
        const float d11 = glm::dot(first, first);
        const float d12 = glm::dot(first, second);
        const float d22 = glm::dot(second, second);
        const float determinant = (d11 * d22) - (d12 * d12);
        const float w1 = ((d22 * glm::dot(at, first)) - (d12 * glm::dot(at, second))) / determinant;
        const float w2 = ((d11 * glm::dot(at, second)) - (d12 * glm::dot(at, first))) / determinant;
        const std::size_t bin = binOf(glm::vec2 { w1 + w2, w2 / (w1 + w2) });
        if (!drawn.found() || bin >= observed.size()) {
            out.outside++;
            continue;
        }
        observed[bin] += 1.0;
    }
    for (std::size_t bin = 0; bin < expected.size(); ++bin) {
        out.total += expected[bin];
        const double count = expected[bin] * static_cast<double>(kDraws);
        if (count >= 5.0) {
            out.chiSquared += (observed[bin] - count) * (observed[bin] - count) / count;
            out.bins++;
        }
    }
    return out;
}

// a triangle of a mesh light is sampled in the solid angle it subtends, near and at a slant, and
// by its area where it subtends too little. It draws nothing for a point behind it
BOOST_AUTO_TEST_CASE(moya_lightsampling_a_mesh_lights_triangle_samples_follow_their_density_test) {
    const glm::vec3 corners[3] = { glm::vec3 { 0.0F, 0.0F, 0.0F }, glm::vec3 { 1.0F, 0.0F, 0.0F }, glm::vec3 { 0.2F, 0.9F, 0.0F } };
    const glm::vec3 near { 0.3F, 0.3F, 0.4F };
    BOOST_TEST(light::spherical::bySolidAngle(light::spherical::triangleAngle(corners, near)));
    holds(fittedTriangle(corners, near), "near");
    holds(fittedTriangle(corners, glm::vec3 { 2.0F, 1.5F, 0.3F }), "slanted");
    const glm::vec3 far { 0.4F, 0.3F, 40.0F };
    BOOST_TEST(!light::spherical::bySolidAngle(light::spherical::triangleAngle(corners, far)));
    holds(fittedTriangle(corners, far), "far");
    const glm::vec3 normal { 0.0F, 0.0F, 1.0F };
    BOOST_TEST(!light::spherical::triangleSample(corners, normal, glm::vec3 { 0.3F, 0.3F, -0.4F }, glm::vec2 { 0.3F, 0.6F }).found());
    BOOST_TEST(light::spherical::triangleDensity(corners, normal, glm::vec3 { 0.3F, 0.3F, -0.4F }, glm::vec3 { 0.3F, 0.3F, 0.0F }) == 0.0F);
}

// a rect or a disk draws nothing for a point behind it, and its density there is zero
BOOST_AUTO_TEST_CASE(moya_lightsampling_nothing_is_drawn_behind_a_light_test) {
    const glm::mat4 place = slanted();
    for (const light::Shape kind : { light::Shape::Rect, light::Shape::Disk }) {
        const light::Light flat = shaped(kind, glm::vec2 { 1.0F, 1.0F });
        const glm::vec3 behind = placed(place, { 0.1F, 0.1F, 1.0F });
        BOOST_TEST(!light::sample(flat, place, behind, glm::vec2 { 0.3F, 0.6F }).found());
        BOOST_TEST(light::density(flat, place, behind, placed(place, { 0.0F, 0.0F, 0.0F })) == 0.0F);
    }
}

};  // namespace

};  // namespace v3d::moya::tracing
