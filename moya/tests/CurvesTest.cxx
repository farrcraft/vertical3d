/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/colour/RgbTables.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/device/cpu/Isa.h>
#include <moya/libmoya/device/CpuLibrary.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/geometry/Curves.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/geometry/Segments.h>
#include <moya/libmoya/geometry/Surfaces.h>
#include <moya/libmoya/kernel/bsdf/Bsdf.h>
#include <moya/libmoya/kernel/bsdf/Layering.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/primitive/Curves.h>
#include <moya/libmoya/kernel/geometry/primitive/ShapeHit.h>
#include <moya/libmoya/kernel/geometry/primitive/Shapes.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>
#include <moya/libmoya/kernel/surface/Frame.h>
#include <moya/libmoya/kernel/surface/OpenPbr.h>
#include <moya/libmoya/kernel/surface/ShadingPoint.h>
#include <moya/libmoya/kernel/surface/Surfaces.h>
#include <moya/libmoya/material/Albedos.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>
#include <moya/libmoya/view/Hits.h>
#include <moya/libmoya/view/Traced.h>
#include <moya/libmoya/view/Tracing.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
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

namespace cpu = v3d::moya::device::cpu;
namespace geometry = v3d::moya::geometry;
namespace kernel = v3d::moya::kernel;
namespace primitive = v3d::moya::kernel::geometry::primitive;
namespace scene = v3d::moya::scene;
namespace view = v3d::moya::view;
typedef v3d::moya::device::CpuDevice CpuDevice;
typedef primitive::Form Form;

/** RI's five bases, by the names the scene gives them. **/
std::vector<std::string> bases() {
    return { "bezier", "b-spline", "catmull-rom", "hermite", "power" };
}

/** A matrix from its entries, rows first, as RI writes a basis. **/
glm::dmat4 rows(const std::array<double, 16> & entries) {
    glm::dmat4 out(0.0);
    for (std::size_t row = 0; row < 4; ++row) {
        for (std::size_t column = 0; column < 4; ++column) {
            out[static_cast<glm::length_t>(column)][static_cast<glm::length_t>(row)] = entries[(row * 4) + column];
        }
    }
    return out;
}

/** RI's matrix for a basis, from the specification, written out apart from libmoya's. **/
glm::dmat4 basisMatrix(const std::string & basis) {
    if (basis == "b-spline") {
        return rows({ -1.0, 3.0, -3.0, 1.0, 3.0, -6.0, 3.0, 0.0, -3.0, 0.0, 3.0, 0.0, 1.0, 4.0, 1.0, 0.0 }) * (1.0 / 6.0);
    }
    if (basis == "catmull-rom") {
        return rows({ -1.0, 3.0, -3.0, 1.0, 2.0, -5.0, 4.0, -1.0, -1.0, 0.0, 1.0, 0.0, 0.0, 2.0, 0.0, 0.0 }) * 0.5;
    }
    if (basis == "hermite") {
        return rows({ 2.0, 1.0, -2.0, 1.0, -3.0, -2.0, 3.0, -1.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0 });
    }
    if (basis == "power") {
        return glm::dmat4(1.0);
    }
    return rows({ -1.0, 3.0, -3.0, 1.0, 3.0, -6.0, 3.0, 0.0, -3.0, 3.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0 });
}

/** The four vertices in a basis of the curve that four Bézier control points describe. **/
std::vector<glm::vec3> inBasis(const std::string & basis, const std::vector<glm::vec3> & bezier) {
    const glm::dmat4 convert = glm::inverse(basisMatrix(basis)) * basisMatrix("bezier");
    std::vector<glm::vec3> out;
    for (int row = 0; row < 4; ++row) {
        glm::dvec3 vertex { 0.0 };
        for (int column = 0; column < 4; ++column) {
            vertex += glm::dvec3(bezier[static_cast<std::size_t>(column)]) * convert[column][row];
        }
        out.emplace_back(vertex);
    }
    return out;
}

/** An arched cubic Bézier segment whose radius grows and then narrows along it. **/
std::array<glm::vec4, 4> arch() {
    return { glm::vec4 { -1.0F, 0.0F, 0.0F, 0.1F }, glm::vec4 { -0.3F, 1.0F, 0.5F, 0.2F }, glm::vec4 { 0.4F, -0.8F, -0.3F, 0.25F },
        glm::vec4 { 1.0F, 0.2F, 0.0F, 0.15F } };
}

/** The arch's centre line, as the four control points of a Bézier curve. **/
std::vector<glm::vec3> archLine() {
    std::vector<glm::vec3> out;
    for (const glm::vec4 & control : arch()) {
        out.emplace_back(control);
    }
    return out;
}

/** How many of a list of hits found something. **/
std::size_t found(const std::vector<kernel::geometry::Hit> & hits) {
    return static_cast<std::size_t>(std::ranges::count_if(hits, [](const kernel::geometry::Hit & hit) {
        return hit.found();
    }));
}

/** How many of two lists of hits differ: one met something and one not, or at another distance. **/
std::size_t differ(const std::vector<kernel::geometry::Hit> & want, const std::vector<kernel::geometry::Hit> & got) {
    std::size_t out = 0;
    for (std::size_t i = 0; i < want.size(); ++i) {
        const bool same = want[i].found() == got[i].found();
        out += same && (!want[i].found() || std::fabs(want[i].distance - got[i].distance) < 1.0e-4F) ? 0 : 1;
    }
    return out;
}

/** A curve's centre and radius along it: linear for two control points, and Bézier for four. **/
glm::vec4 centreAt(const glm::vec4 * controls, std::uint32_t count, float along) {
    return count == 2 ? controls[0] + ((controls[1] - controls[0]) * along) : primitive::bezierAt(controls, along);
}

/** How far a point lies outside the nearest of the spheres of a curve's radius spaced along it. **/
float offSurface(const glm::vec4 * controls, std::uint32_t count, const glm::vec3 & point) {
    float out = std::numeric_limits<float>::infinity();
    for (int step = 0; step <= 4000; ++step) {
        const glm::vec4 sphere = centreAt(controls, count, static_cast<float>(step) / 4000.0F);
        out = std::min(out, glm::length(point - glm::vec3(sphere)) - sphere.w);
    }
    return out;
}

/**
 * Where a ray first enters the spheres of a curve's radius, closely spaced along it, or infinity.
 * The swept surface is where the ray first enters any of them.
 **/
float entersSpheres(const glm::vec4 * controls, std::uint32_t count, const kernel::geometry::Ray & ray) {
    float out = std::numeric_limits<float>::infinity();
    for (int step = 0; step <= 4000; ++step) {
        const glm::vec4 sphere = centreAt(controls, count, static_cast<float>(step) / 4000.0F);
        const glm::vec3 offset = ray.origin - glm::vec3(sphere);
        const float squared = glm::dot(ray.direction, ray.direction);
        const float half = glm::dot(offset, ray.direction);
        const float gap = (half * half) - (squared * (glm::dot(offset, offset) - (sphere.w * sphere.w)));
        if (gap >= 0.0F) {
            out = std::min(out, (-half - std::sqrt(gap)) / squared);
        }
    }
    return out;
}

/** Rays from five away to points inside a curve near its centre line, each a random length. **/
std::vector<kernel::geometry::Ray> towards(const glm::vec4 * controls, std::uint32_t count, std::uint32_t seed) {
    std::vector<kernel::geometry::Ray> out;
    for (int i = 0; i < 400; ++i) {
        const glm::vec4 centre = centreAt(controls, count, next(seed));
        // half the radius from the centre line at most, so no ray only grazes the curve
        const glm::vec3 aim = glm::vec3(centre) + (glm::normalize(glm::vec3 { next(seed), next(seed), next(seed) } - 0.5F) * (0.5F * centre.w * next(seed)));
        const glm::vec3 from = glm::normalize(glm::vec3 { next(seed), next(seed), next(seed) } - 0.5F);
        kernel::geometry::Ray ray;
        ray.origin = aim + (from * 5.0F);
        ray.direction = -from * (0.5F + (2.0F * next(seed)));
        out.push_back(ray);
    }
    return out;
}

/** Curves of a type, basis, wrap and profile through points, a curve per count of vertices. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the scene's own names for each
scene::Handle curves(scene::Scene & scene, const std::string & name, const std::vector<glm::vec3> & points, const std::vector<std::int32_t> & vertices,
    const std::string & type, const std::string & basis, const std::string & profile) {
    const scene::Handle out = scene.create("curves", name);
    scene.set(out, "P", scene::Value::points(points));
    scene.set(out, "vertices", scene::Value::integers(vertices));
    scene.set(out, "type", scene::Value::string(type));
    scene.set(out, "basis", scene::Value::string(basis));
    scene.set(out, "profile", scene::Value::string(profile));
    return out;
}

/** The prototypes of a scene with one instance of each of its prototypes, at the origin. **/
geometry::Prototypes placedOnce(scene::Scene & scene, const std::vector<scene::Handle> & prototypes) {
    std::uint32_t count = 0;
    for (const scene::Handle & prototype : prototypes) {
        instanced(scene, "instance " + std::to_string(count++), prototype, {}, glm::mat4(1.0F));
    }
    scene.commit();
    scene.apply();
    return geometry::gather(scene, {});
}

// a curve given in each of RI's bases converts to the Bézier segment it describes, and its
// varying widths become radii linear along it
BOOST_AUTO_TEST_CASE(moya_curves_each_basis_converts_to_the_bezier_segment_it_describes_test) {
    const std::vector<glm::vec3> bezier = archLine();
    for (const std::string & basis : bases()) {
        const geometry::Segments segments = geometry::cut(inBasis(basis, bezier), { 0.2F, 0.4F }, { 4 }, true, basis, false);
        BOOST_REQUIRE_MESSAGE(segments.controls.size() == 4U, basis);
        float off = 0.0F;
        for (std::size_t control = 0; control < 4; ++control) {
            off = std::max(off, glm::length(glm::vec3(segments.controls[control]) - bezier[control]));
            const float radius = 0.1F + (0.1F * static_cast<float>(control) / 3.0F);
            off = std::max(off, std::fabs(segments.controls[control].w - radius));
        }
        BOOST_TEST(off < 1.0e-5F, basis << ": " << off);
    }
}

// a curve has as many segments as RI gives it, and as many varying values. A periodic curve
// closes on itself, and counts that make no whole segments are refused
BOOST_AUTO_TEST_CASE(moya_curves_a_curve_has_the_segments_ri_gives_it_test) {
    BOOST_TEST(geometry::varyingCount({ 7, 4 }, true, "bezier", false) == 5U);
    BOOST_TEST(geometry::varyingCount({ 6 }, true, "b-spline", false) == 4U);
    BOOST_TEST(geometry::varyingCount({ 6 }, true, "b-spline", true) == 6U);
    BOOST_TEST(geometry::varyingCount({ 6 }, true, "hermite", true) == 3U);
    BOOST_TEST(geometry::varyingCount({ 3, 2 }, false, "bezier", true) == 5U);
    BOOST_CHECK_THROW(geometry::varyingCount({ 6 }, true, "bezier", false), std::invalid_argument);
    BOOST_CHECK_THROW(geometry::varyingCount({ 5 }, true, "hermite", true), std::invalid_argument);
    BOOST_CHECK_THROW(geometry::varyingCount({ 1 }, false, "bezier", false), std::invalid_argument);

    std::vector<glm::vec3> ring;
    ring.reserve(6);
    for (int vertex = 0; vertex < 6; ++vertex) {
        ring.emplace_back(std::cos(static_cast<float>(vertex)), std::sin(static_cast<float>(vertex)), 0.0F);
    }
    const geometry::Segments closed = geometry::cut(ring, std::vector<float>(6, 0.1F), { 6 }, true, "b-spline", true);
    BOOST_REQUIRE(closed.controls.size() == 24U);
    BOOST_TEST(glm::length(closed.controls[23] - closed.controls[0]) < 1.0e-5F);

    const geometry::Segments lines = geometry::cut({ glm::vec3(0.0F), glm::vec3(1.0F), glm::vec3(2.0F), glm::vec3(5.0F), glm::vec3(6.0F) },
        { 1.0F, 1.0F, 1.0F, 1.0F, 1.0F }, { 3, 2 }, false, "bezier", false);
    BOOST_TEST((lines.curves == std::vector<std::uint32_t> { 0, 0, 1 }));
    BOOST_TEST(lines.controls.size() == 6U);
    BOOST_CHECK_THROW(geometry::cut(ring, { 0.1F }, { 6 }, true, "b-spline", true), std::invalid_argument);
    BOOST_CHECK_THROW(geometry::cut(ring, std::vector<float>(6, 0.1F), { 5 }, true, "b-spline", true), std::invalid_argument);
}

// a width at each vertex is converted by the basis as the points are, so equal widths on a
// b-spline give one radius all along it
BOOST_AUTO_TEST_CASE(moya_curves_a_width_at_each_vertex_follows_the_basis_test) {
    const std::vector<glm::vec3> points { { 0.0F, 0.0F, 0.0F }, { 1.0F, 1.0F, 0.0F }, { 2.0F, 0.0F, 0.0F }, { 3.0F, 1.0F, 0.0F }, { 4.0F, 0.0F, 0.0F } };
    const geometry::Segments even = geometry::cut(points, std::vector<float>(5, 0.5F), { 5 }, true, "b-spline", false);
    BOOST_REQUIRE(even.controls.size() == 8U);
    for (const glm::vec4 & control : even.controls) {
        BOOST_TEST(std::fabs(control.w - 0.25F) < 1.0e-6F);
    }
    // a b-spline's ends lie a sixth of the way along from its outer vertices, its widths too
    const geometry::Segments rising = geometry::cut(points, { 0.0F, 0.6F, 1.2F, 1.8F, 2.4F }, { 5 }, true, "b-spline", false);
    BOOST_TEST(std::fabs(rising.controls[0].w - 0.3F) < 1.0e-6F);
    BOOST_TEST(std::fabs(rising.controls[7].w - 0.9F) < 1.0e-6F);
}

// a ray towards a point inside a round curve meets it on the surface swept by a sphere of its
// radius, which is at the radius from the centre line. It meets it no later than it enters any of
// the spheres, linear and cubic. A ray that grazes the curve may meet it within the tolerance
BOOST_AUTO_TEST_CASE(moya_curves_a_round_curves_hit_is_at_its_width_from_its_centre_line_test) {
    const glm::vec4 line[2] = { { -1.0F, 0.0F, 0.0F, 0.1F }, { 1.0F, 0.5F, 0.2F, 0.3F } };
    for (const Form form : { Form::RoundLinearCurves, Form::RoundCubicCurves }) {
        const std::array<glm::vec4, 4> cubic = arch();
        const glm::vec4 * controls = form == Form::RoundLinearCurves ? line : cubic.data();
        const std::uint32_t count = primitive::controlsOf(form);
        std::size_t wrong = 0;
        for (const kernel::geometry::Ray & ray : towards(controls, count, 7)) {
            primitive::ShapeHit hit;
            if (!primitive::intersectCurve(form, controls, true, ray, ray.start, ray.end, hit)) {
                ++wrong;
                continue;
            }
            const float length = glm::length(ray.direction);
            wrong += std::fabs(offSurface(controls, count, ray.at(hit.distance))) > 1.0e-3F ? 1 : 0;
            wrong += hit.distance * length > (entersSpheres(controls, count, ray) * length) + 1.0e-3F ? 1 : 0;
            wrong += hit.uv.x >= 0.0F && hit.uv.x <= 1.0F && hit.uv.y >= 0.0F && hit.uv.y <= 1.0F ? 0 : 1;
        }
        BOOST_TEST(wrong == 0U, (form == Form::RoundLinearCurves ? "linear" : "cubic") << ": " << wrong);
    }
}

// a ray towards a point inside a flat curve meets it level with the centre line, within the
// radius of the point along it that the hit names, linear and cubic
BOOST_AUTO_TEST_CASE(moya_curves_a_flat_curves_hit_faces_the_ray_test) {
    const glm::vec4 line[2] = { { -1.0F, 0.0F, 0.0F, 0.1F }, { 1.0F, 0.5F, 0.2F, 0.3F } };
    for (const Form form : { Form::FlatLinearCurves, Form::FlatCubicCurves }) {
        const std::array<glm::vec4, 4> cubic = arch();
        const glm::vec4 * controls = form == Form::FlatLinearCurves ? line : cubic.data();
        const std::uint32_t count = primitive::controlsOf(form);
        std::size_t wrong = 0;
        for (const kernel::geometry::Ray & ray : towards(controls, count, 11)) {
            primitive::ShapeHit hit;
            if (!primitive::intersectCurve(form, controls, true, ray, ray.start, ray.end, hit)) {
                ++wrong;
                continue;
            }
            const glm::vec4 centre = centreAt(controls, count, hit.uv.y);
            const glm::vec3 across = ray.at(hit.distance) - glm::vec3(centre);
            wrong += std::fabs(glm::dot(across, glm::normalize(ray.direction))) > 1.0e-4F ? 1 : 0;
            wrong += glm::length(across) > centre.w + 1.0e-4F ? 1 : 0;
            const glm::vec3 normal = primitive::curveNormal(form, controls, ray.at(hit.distance), hit.uv.y, ray.direction);
            wrong += glm::length(glm::normalize(normal) + glm::normalize(ray.direction)) > 1.0e-5F ? 1 : 0;
        }
        BOOST_TEST(wrong == 0U, (form == Form::FlatLinearCurves ? "linear" : "cubic") << ": " << wrong);
    }
    // a ray inside the radius at a widening segment's wide end, but outside the radius of the
    // point nearest it, meets the segment
    const glm::vec4 widening[4] = { { 0.0F, 0.0F, 5.0F, 0.05F }, { 1.0F / 3.0F, 0.0F, 5.0F, 0.3F }, { 2.0F / 3.0F, 0.0F, 5.0F, 0.55F },
        { 1.0F, 0.0F, 5.0F, 0.8F } };
    const glm::vec4 wideLine[2] = { widening[0], widening[3] };
    kernel::geometry::Ray ray;
    ray.origin = glm::vec3 { 0.6F, 0.6F, 0.0F };
    ray.direction = glm::vec3 { 0.0F, 0.0F, 1.0F };
    for (const Form form : { Form::FlatLinearCurves, Form::FlatCubicCurves }) {
        primitive::ShapeHit hit;
        BOOST_TEST(primitive::intersectCurve(form, form == Form::FlatLinearCurves ? wideLine : widening, true, ray, ray.start, ray.end, hit),
            (form == Form::FlatLinearCurves ? "linear" : "cubic"));
    }
}

// one curve given in each basis is hit where the Bézier curve is, round and flat, through every
// traversal
BOOST_AUTO_TEST_CASE(moya_curves_a_curve_in_each_basis_is_hit_where_the_bezier_curve_is_test) {
    const std::vector<glm::vec3> bezier = archLine();
    std::uint32_t seed = 5;
    std::vector<kernel::geometry::Ray> rays;
    for (int i = 0; i < 300; ++i) {
        kernel::geometry::Ray ray;
        ray.origin = glm::vec3 { (next(seed) * 2.4F) - 1.2F, (next(seed) * 2.0F) - 1.0F, -5.0F };
        ray.direction = glm::vec3 { (next(seed) - 0.5F) * 0.2F, (next(seed) - 0.5F) * 0.2F, 1.0F };
        rays.push_back(ray);
    }
    for (const std::string profile : { "round", "flat" }) {
        std::vector<kernel::geometry::Hit> want;
        for (const std::string & basis : bases()) {
            scene::Scene scene(scene::kinds());
            const scene::Handle hair = curves(scene, "hair", inBasis(basis, bezier), { 4 }, "cubic", basis, profile);
            scene.set(hair, "constantWidth", scene::Value::number(0.3F));
            const geometry::Prototypes prototypes = placedOnce(scene, { hair });
            BOOST_REQUIRE(prototypes.reports.empty());
            eachTraversal([&](view::Tracer tracer, cpu::Isa isa) {
                CpuDevice on(0, isa);
                const view::Traced traced = view::trace(on, prototypes, rays, tracer);
                if (want.empty()) {
                    want = traced.nearest;
                    BOOST_TEST(found(want) > rays.size() / 10);
                    return;
                }
                const std::size_t wrong = differ(want, traced.nearest);
                BOOST_TEST(wrong == 0U, profile << ", " << basis << ", " << tracerName(tracer, isa) << ": " << wrong);
            });
        }
    }
}

// rays spread evenly across a curve a tenth as wide as their spread meet it a tenth of the time,
// at every form and traversal. A thin curve is never lost to the tests' tolerances
BOOST_AUTO_TEST_CASE(moya_curves_a_thin_curve_is_met_in_proportion_to_its_width_test) {
    constexpr std::size_t kRays = 2000;
    constexpr float kSpread = 0.01F;
    std::vector<kernel::geometry::Ray> rays;
    for (std::size_t i = 0; i < kRays; ++i) {
        kernel::geometry::Ray ray;
        ray.origin = glm::vec3 { -0.5F + (static_cast<float>(i % 7) * 0.15F), (kSpread * ((static_cast<float>(i) + 0.5F) / kRays)) - (kSpread * 0.5F), 0.0F };
        ray.direction = glm::vec3 { 0.0F, 0.0F, 1.0F };
        rays.push_back(ray);
    }
    for (const std::string type : { "linear", "cubic" }) {
        for (const std::string profile : { "round", "flat" }) {
            scene::Scene scene(scene::kinds());
            const std::vector<glm::vec3> points = type == "linear" ?
                std::vector<glm::vec3> { { -1.0F, 0.0F, 10.0F }, { 0.0F, 0.0F, 9.0F }, { 1.0F, 0.0F, 10.0F } } :
                std::vector<glm::vec3> { { -1.0F, 0.0F, 10.0F }, { -0.3F, 0.0F, 9.0F }, { 0.3F, 0.0F, 11.0F }, { 1.0F, 0.0F, 10.0F } };
            const scene::Handle hair = curves(scene, "hair", points, { static_cast<std::int32_t>(points.size()) }, type, "bezier", profile);
            scene.set(hair, "constantWidth", scene::Value::number(kSpread / 10.0F));
            const geometry::Prototypes prototypes = placedOnce(scene, { hair });
            eachTraversal([&](view::Tracer tracer, cpu::Isa isa) {
                CpuDevice on(0, isa);
                const view::Traced traced = view::trace(on, prototypes, rays, tracer);
                const std::size_t hits = found(traced.nearest);
                BOOST_TEST((hits >= (kRays / 10) - 2 && hits <= (kRays / 10) + 2), type << ", " << profile << ", " << tracerName(tracer, isa) << ": " << hits);
            });
        }
    }
}

// two segments share the point where one ends and the next starts, and a ray through it meets the
// next, round and flat, through every traversal. Each end of the curve is its own segment's
BOOST_AUTO_TEST_CASE(moya_curves_a_joint_is_held_by_the_segment_after_it_test) {
    for (const std::string profile : { "round", "flat" }) {
        scene::Scene scene(scene::kinds());
        const scene::Handle hair =
            curves(scene, "hair", { { -1.0F, 0.0F, 5.0F }, { 0.0F, 0.5F, 5.0F }, { 1.0F, 0.0F, 5.0F } }, { 3 }, "linear", "bezier", profile);
        scene.set(hair, "constantWidth", scene::Value::number(0.4F));
        const geometry::Prototypes prototypes = placedOnce(scene, { hair });
        std::vector<kernel::geometry::Ray> rays;
        for (const float x : { -1.0F, 0.0F, 1.0F }) {
            kernel::geometry::Ray ray;
            ray.origin = glm::vec3 { x, x == 0.0F ? 0.5F : 0.0F, 0.0F };
            ray.direction = glm::vec3 { 0.0F, 0.0F, 1.0F };
            rays.push_back(ray);
        }
        eachTraversal([&](view::Tracer tracer, cpu::Isa isa) {
            CpuDevice on(0, isa);
            const view::Traced traced = view::trace(on, prototypes, rays, tracer);
            const std::uint32_t want[3] = { 0, 1, 1 };
            for (std::size_t i = 0; i < rays.size(); ++i) {
                BOOST_TEST(traced.nearest[i].primitive == want[i], profile << ", " << tracerName(tracer, isa) << ", ray " << i);
            }
        });
    }
}

// a curve whose points move is met where it stands at the ray's time
BOOST_AUTO_TEST_CASE(moya_curves_a_moving_curve_is_met_where_it_stands_at_the_rays_time_test) {
    scene::Scene scene(scene::kinds());
    shutter(scene);
    const std::vector<glm::vec3> start { { -1.0F, 0.0F, 5.0F }, { -0.3F, 0.0F, 5.0F }, { 0.3F, 0.0F, 5.0F }, { 1.0F, 0.0F, 5.0F } };
    std::vector<glm::vec3> finish = start;
    for (glm::vec3 & point : finish) {
        point.y += 1.0F;
    }
    const scene::Handle hair = curves(scene, "hair", start, { 4 }, "cubic", "bezier", "round");
    scene.set(hair, "P", { 0.0F, 1.0F }, { scene::Value::points(start), scene::Value::points(finish) });
    scene.set(hair, "constantWidth", scene::Value::number(0.2F));
    const geometry::Prototypes prototypes = placedOnce(scene, { hair });
    std::vector<kernel::geometry::Ray> rays;
    for (const float time : { 0.0F, 0.5F, 1.0F }) {
        for (const float height : { 0.0F, 0.5F, 1.0F }) {
            kernel::geometry::Ray ray;
            ray.origin = glm::vec3 { 0.2F, height, 0.0F };
            ray.direction = glm::vec3 { 0.0F, 0.0F, 1.0F };
            ray.time = time;
            rays.push_back(ray);
        }
    }
    eachTraversal([&](view::Tracer tracer, cpu::Isa isa) {
        CpuDevice on(0, isa);
        const view::Traced traced = view::trace(on, prototypes, rays, tracer);
        for (std::size_t i = 0; i < rays.size(); ++i) {
            // the ray at the curve's height at its time meets its front, and the others miss
            const bool level = i / 3 == i % 3;
            BOOST_TEST(traced.nearest[i].found() == level, tracerName(tracer, isa) << ", ray " << i);
            if (level) {
                BOOST_TEST(std::fabs(traced.nearest[i].distance - 4.9F) < 1.0e-4F, tracerName(tracer, isa) << ", ray " << i);
            }
        }
    });
}

// curves that cannot be cut into segments are left out and reported, and curves whose widths
// are neither one per vertex nor one per varying value take their constant width
BOOST_AUTO_TEST_CASE(moya_curves_curves_that_cannot_be_cut_are_reported_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle broken = curves(scene, "broken", std::vector<glm::vec3>(5, glm::vec3(0.0F)), { 5 }, "cubic", "bezier", "round");
    const scene::Handle tooShort = curves(scene, "short", std::vector<glm::vec3>(4, glm::vec3(0.0F)), { 3 }, "linear", "bezier", "round");
    const scene::Handle uneven = curves(scene, "uneven", std::vector<glm::vec3>(4, glm::vec3(0.0F)), { 4 }, "cubic", "bezier", "round");
    scene.set(uneven, "width", scene::Value::floats({ 1.0F, 2.0F, 3.0F }));
    scene.set(uneven, "constantWidth", scene::Value::number(0.5F));
    const geometry::Prototypes prototypes = placedOnce(scene, { broken, tooShort, uneven });
    BOOST_TEST(reported(prototypes, "curves broken is left out, because a cubic curve of 5 vertices"));
    BOOST_TEST(reported(prototypes, "curves short is left out, because its counts of vertices do not add up to its points"));
    BOOST_TEST(reported(prototypes, "curves uneven has a width for some of its vertices but not all"));
    BOOST_REQUIRE(prototypes.geometries.size() == 1U);
    BOOST_TEST(prototypes.instances.size() == 1U);
    BOOST_TEST(prototypes.controls.size() == 4U);
    for (const glm::vec4 & control : prototypes.controls) {
        BOOST_TEST(control.w == 0.25F);
    }
}

/**
 * How many vertices make a curve of seven segments in a basis, or ten for a basis of step 1. A
 * linear curve's basis is "linear".
 **/
std::int32_t sevenSegments(const std::string & basis) {
    if (basis == "linear") {
        return 8;
    }
    if (basis == "bezier") {
        return 22;
    }
    if (basis == "hermite") {
        return 16;
    }
    if (basis == "power") {
        return 28;
    }
    return 10;
}

/**
 * Twelve random curves of a profile in a basis, or linear for the basis "linear". Their points
 * move over the shutter and their widths narrow, each on its own times.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the profile, then the basis
scene::Handle movingHair(scene::Scene & scene, const std::string & profile, const std::string & basis, std::uint32_t & seed) {
    const bool cubic = basis != "linear";
    const std::int32_t each = sevenSegments(basis);
    // a basis whose vertices include tangents or coefficients takes smaller steps
    const float stride = basis == "power" || basis == "hermite" ? 0.5F : 1.5F;
    std::vector<glm::vec3> start;
    std::vector<glm::vec3> finish;
    std::vector<float> widths;
    for (int curve = 0; curve < 12; ++curve) {
        glm::vec3 at = glm::vec3 { next(seed), next(seed), next(seed) } * 10.0F;
        for (std::int32_t vertex = 0; vertex < each; ++vertex) {
            at += (glm::vec3 { next(seed), next(seed), next(seed) } - 0.5F) * stride;
            start.push_back(at);
            finish.push_back(at + ((glm::vec3 { next(seed), next(seed), next(seed) } - 0.5F) * 1.5F));
            widths.push_back(0.05F + (0.3F * next(seed)));
        }
    }
    std::string name = profile;
    name += " ";
    name += basis;
    const scene::Handle out = curves(scene, name, start, std::vector<std::int32_t>(12, each), cubic ? "cubic" : "linear", cubic ? basis : "bezier", profile);
    scene.set(out, "P", { 0.0F, 1.0F }, { scene::Value::points(start), scene::Value::points(finish) });
    std::vector<float> narrower = widths;
    for (float & width : narrower) {
        width *= 0.5F;
    }
    scene.set(out, "width", { 0.0F, 0.6F }, { scene::Value::floats(widths), scene::Value::floats(narrower) });
    return out;
}

// a scene holds curves of every form and basis, moving, stretched, or both. The hierarchy hits
// it as the brute-force traversal hits the scene placed at each of several times, at every
// instruction set
BOOST_AUTO_TEST_CASE(moya_curves_the_hierarchy_finds_the_oracles_nearest_hit_on_curves_test) {
    std::uint32_t seed = 23;
    scene::Scene scene(scene::kinds());
    shutter(scene);
    std::vector<scene::Handle> prototypes;
    for (const std::string profile : { "round", "flat" }) {
        for (const std::string basis : { "linear", "bezier", "b-spline", "catmull-rom", "hermite", "power" }) {
            prototypes.push_back(movingHair(scene, profile, basis, seed));
        }
    }
    std::uint32_t count = 0;
    for (const scene::Handle & prototype : prototypes) {
        for (int copy = 0; copy < 2; ++copy) {
            const glm::mat4 place = glm::translate(glm::mat4(1.0F), glm::vec3 { next(seed), next(seed), next(seed) } * 15.0F) *
                glm::rotate(glm::mat4(1.0F), next(seed) * 6.0F, glm::normalize(glm::vec3 { next(seed), next(seed), next(seed) } + 0.1F)) *
                glm::scale(glm::mat4(1.0F), glm::vec3 { 0.5F + next(seed), 0.5F + (2.0F * next(seed)), 1.0F });
            const std::string name = "copy " + std::to_string(count++);
            if (copy == 1) {
                instanced(scene, name, prototype, {}, { 0.0F, 1.0F }, { place, glm::translate(place, glm::vec3 { 2.0F, 0.0F, 1.0F }) });
            } else {
                instanced(scene, name, prototype, {}, {}, { place });
            }
        }
    }
    scene.commit();
    scene.apply();
    const geometry::Prototypes gathered = geometry::gather(scene, {});
    BOOST_TEST(gathered.reports.empty());

    const std::vector<float> times { 0.0F, 0.2F, 0.45F, 0.7F, 1.0F };
    std::vector<kernel::geometry::Ray> rays;
    for (std::size_t i = 0; i < 3000; ++i) {
        kernel::geometry::Ray ray;
        ray.origin = glm::vec3 { next(seed), next(seed), next(seed) } * 30.0F - 5.0F;
        ray.direction = glm::vec3 { next(seed), next(seed), next(seed) } * 2.0F - 1.0F;
        ray.time = times[i % times.size()];
        rays.push_back(ray);
    }
    CpuDevice device;
    const view::Traced reference = view::trace(device, gathered, rays, view::Tracer::BruteForce);
    // the scene is hit often enough that agreeing says something
    BOOST_TEST(found(reference.nearest) > rays.size() / 10);
    for (const cpu::Isa isa : runnable()) {
        CpuDevice on(0, isa);
        const view::Traced traced = view::trace(on, gathered, rays, view::Tracer::Hierarchy);
        std::size_t disagreements = 0;
        for (std::size_t i = 0; i < rays.size(); ++i) {
            if (!agrees(reference.nearest[i], traced.nearest[i]) || reference.any[i] != traced.any[i]) {
                ++disagreements;
            }
        }
        BOOST_TEST(disagreements == 0U, v3d::moya::device::isaName(isa) << ": " << disagreements);
    }
}

// the hit view of a curve across an orthographic camera's view names the curve as its face. A
// flat curve faces the camera everywhere it is met, and a round one turns away above its centre
BOOST_AUTO_TEST_CASE(moya_curves_the_hit_view_gives_a_curves_face_and_normal_test) {
    constexpr std::int32_t kSide = 15;
    for (const std::string profile : { "flat", "round" }) {
        scene::Scene scene(scene::kinds());
        const scene::Handle options = scene.create("options", "options");
        const scene::Handle camera = scene.create("camera", "camera");
        scene.set(options, "resolution", scene::Value::integers({ kSide, kSide }));
        scene.set(options, "camera", scene::Value::handle(camera));
        scene.set(camera, "projection", scene::Value::string("orthographic"));
        // a curve of two segments first, out of view, so the one in view is the second curve and
        // its third segment
        const scene::Handle hair =
            curves(scene, "hair", { { 5.0F, 5.0F, 5.0F }, { 6.0F, 5.0F, 5.0F }, { 7.0F, 5.0F, 5.0F }, { -2.0F, 0.0F, 5.0F }, { 2.0F, 0.0F, 5.0F } },
                { 3, 2 }, "linear", "bezier", profile);
        scene.set(hair, "constantWidth", scene::Value::number(1.0F));
        instanced(scene, "hair", hair, {}, glm::mat4(1.0F));
        scene.commit();
        scene.apply();
        for (const view::Tracer tracer : { view::Tracer::BruteForce, view::Tracer::Hierarchy }) {
            CpuDevice device;
            const v3d::moya::film::Snapshot snapshot = view::hits(device, scene, {}, 0.0F, tracer);
            const std::uint32_t middle = kSide / 2;
            BOOST_TEST(snapshot.at(middle, middle, snapshot.offset(view::kFace)) == 1.0F, profile);
            BOOST_TEST(snapshot.at(middle, 0, snapshot.offset(view::kCoverage)) == 0.0F, profile);
            // two rows above the middle, 0.27 above the centre line of a curve 0.5 in radius
            const float rising = snapshot.at(middle, middle - 2, snapshot.offset(view::kNormal) + 1);
            const float facing = snapshot.at(middle, middle - 2, snapshot.offset(view::kNormal) + 2);
            if (profile == "flat") {
                BOOST_TEST(std::fabs(facing + 1.0F) < 1.0e-5F);
                BOOST_TEST(std::fabs(snapshot.at(middle, middle - 2, snapshot.offset(view::kDepth)) - 5.0F) < 1.0e-4F);
            } else {
                BOOST_TEST(std::fabs(rising - (0.8F / 1.5F)) < 1.0e-3F, rising);
                BOOST_TEST(facing < -0.8F);
            }
        }
    }
}

// a curve's direction is its centre line's derivative: a linear segment's chord, and a cubic
// one's slope, which a difference along it agrees with
BOOST_AUTO_TEST_CASE(moya_curves_a_curves_direction_is_its_centre_lines_derivative_test) {
    // room for a cubic's four points, of which a linear segment reads the first two
    const glm::vec4 line[4] = { { -1.0F, 0.0F, 0.0F, 0.1F }, { 1.0F, 0.5F, 0.2F, 0.3F }, {}, {} };
    for (const Form form : { Form::RoundLinearCurves, Form::FlatLinearCurves }) {
        const glm::vec3 direction = primitive::curveDirection(form, line, 0.3F);
        BOOST_TEST(glm::length(direction - glm::vec3 { 2.0F, 0.5F, 0.2F }) < 1.0e-6F);
    }
    const std::array<glm::vec4, 4> cubic = arch();
    for (const float along : { 0.0F, 0.25F, 0.6F, 1.0F }) {
        const float step = 1.0e-3F;
        const float low = std::fmax(0.0F, along - step);
        const float high = std::fmin(1.0F, along + step);
        const glm::vec3 difference = glm::vec3(centreAt(cubic.data(), 4, high) - centreAt(cubic.data(), 4, low)) / (high - low);
        for (const Form form : { Form::RoundCubicCurves, Form::FlatCubicCurves }) {
            const glm::vec3 direction = primitive::curveDirection(form, cubic.data(), along);
            BOOST_TEST(glm::length(direction - difference) < 2.0e-2F * glm::length(difference), along);
        }
    }
    BOOST_TEST(glm::length(primitive::curveDirection(Form::Triangles, line, 0.5F)) == 0.0F);
}

/**
 * The shading points of rays straight up through a straight curve of a type and profile, half a
 * unit in radius and five units up. It runs from -2 to 2 along a line turned 30 degrees about z,
 * and each ray passes a place along it, at an offset to its side.
 **/
std::vector<kernel::surface::ShadingPoint> acrossStraight(const std::string & type, const std::string & profile, const std::vector<glm::vec2> & places) {
    scene::Scene scene(scene::kinds());
    const std::vector<glm::vec3> points = type == "linear"
        ? std::vector<glm::vec3> { { -2.0F, 0.0F, 0.0F }, { 2.0F, 0.0F, 0.0F } }
        : std::vector<glm::vec3> { { -2.0F, 0.0F, 0.0F }, { -0.5F, 0.0F, 0.0F }, { 0.5F, 0.0F, 0.0F }, { 2.0F, 0.0F, 0.0F } };
    const scene::Handle hair = curves(scene, "hair", points, { static_cast<std::int32_t>(points.size()) }, type, "bezier", profile);
    scene.set(hair, "constantWidth", scene::Value::number(1.0F));
    const glm::mat4 placed = glm::rotate(moved(0.0F, 0.0F, 5.0F), glm::radians(30.0F), glm::vec3 { 0.0F, 0.0F, 1.0F });
    instanced(scene, "hair", hair, {}, placed);
    scene.commit();
    scene.apply();
    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    const glm::vec3 axis = glm::mat3(placed) * glm::vec3 { 1.0F, 0.0F, 0.0F };
    const glm::vec3 side = glm::mat3(placed) * glm::vec3 { 0.0F, 1.0F, 0.0F };
    std::vector<kernel::geometry::Ray> rays;
    rays.reserve(places.size());
    for (const glm::vec2 & place : places) {
        kernel::geometry::Ray ray;
        ray.origin = (axis * place.x) + (side * place.y);
        ray.direction = glm::vec3 { 0.0F, 0.0F, 1.0F };
        rays.push_back(ray);
    }
    CpuDevice device;
    const view::Traced traced = view::trace(device, prototypes, rays, view::Tracer::Hierarchy);
    const geometry::Surfaces surfaces(device, prototypes, v3d::moya::colour::Config());
    const kernel::surface::Surfaces held = surfaces.view();
    std::vector<kernel::surface::ShadingPoint> out;
    out.reserve(rays.size());
    for (std::size_t i = 0; i < rays.size(); ++i) {
        out.push_back(held.at(traced.nearest[i], rays[i]));
    }
    return out;
}

/** The axis acrossStraight()'s curve runs along. **/
glm::vec3 straightAxis() {
    return glm::mat3(glm::rotate(glm::mat4(1.0F), glm::radians(30.0F), glm::vec3 { 0.0F, 0.0F, 1.0F })) * glm::vec3 { 1.0F, 0.0F, 0.0F };
}

// a straight curve's shading point holds the direction it runs at every hit, turned into the
// world with its instance, and its frame's tangent lies along it. The frame stays at right angles
// about the shading normal. Linear and cubic, round and flat
BOOST_AUTO_TEST_CASE(moya_curves_a_straight_curves_tangent_lies_along_it_test) {
    const glm::vec3 axis = straightAxis();
    const std::vector<glm::vec2> places { { -1.5F, 0.0F }, { -0.4F, 0.3F }, { 0.7F, -0.2F }, { 1.6F, 0.45F } };
    for (const std::string type : { "linear", "cubic" }) {
        for (const std::string profile : { "round", "flat" }) {
            BOOST_TEST_CONTEXT(type << " " << profile) {
                for (const kernel::surface::ShadingPoint & point : acrossStraight(type, profile, places)) {
                    BOOST_TEST_REQUIRE(point.found());
                    BOOST_TEST(glm::dot(point.fibre, axis) > 1.0F - 1.0e-5F);
                    BOOST_TEST(glm::dot(point.frame.tangent, axis) > 1.0F - 1.0e-4F);
                    BOOST_TEST(std::fabs(glm::dot(point.frame.tangent, point.frame.normal)) < 1.0e-5F);
                    BOOST_TEST(glm::length(glm::cross(point.frame.tangent, point.frame.bitangent) - point.frame.normal) < 1.0e-5F);
                    BOOST_TEST(glm::length(point.frame.normal - point.shading) < 1.0e-6F);
                }
            }
        }
    }
}

// a hit's place across a curve is zero on its centre line and runs to one at its edges. It is the
// ray's distance from the centre line over the radius, of one sign on each side
BOOST_AUTO_TEST_CASE(moya_curves_a_hits_place_across_runs_from_edge_to_edge_test) {
    const std::vector<float> offsets { -0.45F, -0.25F, 0.0F, 0.1F, 0.25F, 0.45F };
    std::vector<glm::vec2> places;
    places.reserve(offsets.size());
    for (const float offset : offsets) {
        places.emplace_back(0.3F, offset);
    }
    for (const std::string type : { "linear", "cubic" }) {
        for (const std::string profile : { "round", "flat" }) {
            BOOST_TEST_CONTEXT(type << " " << profile) {
                const std::vector<kernel::surface::ShadingPoint> points = acrossStraight(type, profile, places);
                const float sign = points.back().across > 0.0F ? 1.0F : -1.0F;
                for (std::size_t i = 0; i < points.size(); ++i) {
                    BOOST_TEST_REQUIRE(points[i].found());
                    BOOST_TEST(std::fabs(points[i].across - (sign * offsets[i] / 0.5F)) < 2.0e-3F, offsets[i] << ": " << points[i].across);
                }
            }
        }
    }
}

// an isotropic OpenPBR surface at a curve's point scatters the same in the curve's frame as in
// the frame it had before, about the same shading normal. Turning the frame to the fibre then
// changes no picture but its noise
BOOST_AUTO_TEST_CASE(moya_curves_an_isotropic_surface_on_a_curve_scatters_as_before_test) {
    const kernel::surface::ShadingPoint point = acrossStraight("cubic", "round", { { 0.4F, 0.2F } }).front();
    BOOST_TEST_REQUIRE(point.found());
    const auto built = [&](const kernel::surface::Frame & frame, const glm::vec3 & outgoing) {
        kernel::bsdf::Bsdf out;
        out.frame = frame;
        out.coatFrame = frame;
        out.geometric = point.geometric;
        out.albedos = v3d::moya::material::ggxAlbedos();
        kernel::surface::OpenPbr surface;
        surface.specularRoughness = 0.4F;
        surface.coatWeight = 0.5F;
        kernel::bsdf::Layering::add(out, surface, colour::acescgTable(), kernel::spectrum::SampledWavelengths::visible(0.5F), glm::dot(outgoing, point.shading));
        return out;
    };
    const glm::vec3 outgoing = glm::normalize(point.shading + (point.frame.tangent * 0.6F) + (point.frame.bitangent * 0.2F));
    const kernel::bsdf::Bsdf now = built(point.frame, outgoing);
    const kernel::bsdf::Bsdf then = built(kernel::surface::Frame::around(point.shading), outgoing);
    std::uint32_t seed = 1601;
    for (int i = 0; i < 64; ++i) {
        const glm::vec3 incoming = glm::normalize(point.shading + glm::vec3 { next(seed) - 0.5F, next(seed) - 0.5F, next(seed) - 0.5F });
        const kernel::spectrum::SampledSpectrum one = now.projected(outgoing, incoming);
        const kernel::spectrum::SampledSpectrum other = then.projected(outgoing, incoming);
        for (int w = 0; w < kernel::spectrum::kSamples; ++w) {
            BOOST_TEST(std::fabs(one[w] - other[w]) <= 1.0e-4F * std::fmax(1.0F, std::fabs(other[w])));
        }
    }
}

};  // namespace
};  // namespace v3d::moya::tracing
