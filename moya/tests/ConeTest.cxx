/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/kernel/bsdf/Closure.h>
#include <moya/libmoya/kernel/bsdf/Model.h>
#include <moya/libmoya/kernel/camera/Camera.h>
#include <moya/libmoya/kernel/geometry/Cone.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/surface/Footprint.h>

#include <cmath>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace {

namespace kernel = v3d::moya::kernel;
using kernel::camera::Camera;
using kernel::geometry::Cone;
using kernel::surface::Footprint;

/** The depth of the plane the camera looks at. **/
constexpr float kDepth = 5.0F;

/** A camera of 64 by 48 pixels, whose field of view across is 2 atan(0.5). **/
Camera perspective() {
    Camera out;
    out.perspective = true;
    out.tanHalfFov = 0.5F;
    out.screen = glm::vec4 { -1.0F, 1.0F, -0.75F, 0.75F };
    out.resolution = glm::vec2 { 64.0F, 48.0F };
    out.near = 0.0F;
    out.far = 100.0F;
    return out;
}

/** The footprint a camera's ray through a pixel's centre makes on the plane z = kDepth. **/
Footprint onThePlane(const Camera & camera, const glm::vec2 & pixel) {
    const kernel::geometry::Ray ray = camera.ray(pixel + 0.5F, glm::vec2 { 0.5F }, 0.0F);
    const float length = glm::length(ray.direction);
    // the direction has a depth of one, so the plane lies kDepth of its lengths along it
    const float travelled = (kDepth - ray.origin.z) / ray.direction.z * length;
    return kernel::surface::footprintOf(camera.cone(ray).widthAt(travelled), ray.direction / length, glm::vec3 { 0.0F, 0.0F, -1.0F });
}

// a plane square to a pinhole camera takes a footprint a pixel wide at each pixel, and a pixel
// wide along the ray too at the middle, where the ray meets it straight on
BOOST_AUTO_TEST_CASE(moya_cone_a_plane_square_to_the_camera_is_a_pixel_at_each_pixel_test) {
    const Camera camera = perspective();
    // a pixel's width on the plane: the screen's width at kDepth over the pixels across it
    const float pixel = kDepth * 2.0F * camera.tanHalfFov / camera.resolution.x;
    for (const glm::vec2 & at : { glm::vec2 { 32.0F, 24.0F }, glm::vec2 { 0.0F, 0.0F }, glm::vec2 { 63.0F, 10.0F } }) {
        BOOST_TEST_CONTEXT("pixel " << at.x << ", " << at.y) {
            BOOST_TEST(onThePlane(camera, at).width() == pixel, boost::test_tools::tolerance(1.0e-4F));
        }
    }
    const Footprint middle = onThePlane(camera, glm::vec2 { 31.5F, 23.5F });
    BOOST_TEST(glm::length(middle.major) == pixel, boost::test_tools::tolerance(1.0e-4F));
}

// an orthographic camera's cone is a pixel wide everywhere, and a lens's narrows to a pixel's
// width at the depth in focus
BOOST_AUTO_TEST_CASE(moya_cone_an_orthographic_or_a_lens_cone_test) {
    Camera flat = perspective();
    flat.perspective = false;
    const Cone straight = flat.cone(flat.ray(glm::vec2 { 10.5F, 20.5F }, glm::vec2 { 0.5F }, 0.0F));
    BOOST_TEST(straight.width == 2.0F / 64.0F, boost::test_tools::tolerance(1.0e-6F));
    BOOST_TEST(straight.spread == 0.0F);

    Camera lens = perspective();
    lens.lensRadius = 0.2F;
    lens.focalDistance = kDepth;
    const kernel::geometry::Ray ray = lens.ray(glm::vec2 { 32.5F, 24.5F }, glm::vec2 { 0.9F, 0.3F }, 0.0F);
    const Cone through = lens.cone(ray);
    BOOST_TEST(through.width == 0.4F, boost::test_tools::tolerance(1.0e-6F));
    // the focus lies a depth of kDepth along the ray, each of its lengths a unit of depth
    const float focus = kDepth * glm::length(ray.direction);
    BOOST_TEST(through.widthAt(focus) == kDepth * 2.0F * lens.tanHalfFov / lens.resolution.x, boost::test_tools::tolerance(1.0e-4F));
    BOOST_TEST(through.widthAt(focus * 2.0F) > through.widthAt(focus));
}

// a cone meeting a plane at an angle makes an ellipse as wide as the cone across the ray. Along
// the ray as the plane sees it, the ellipse is the width over the cosine long
BOOST_AUTO_TEST_CASE(moya_cone_a_plane_at_an_angle_stretches_along_the_ray_test) {
    const float angle = std::acos(0.25F);
    const glm::vec3 normal { 0.0F, 0.0F, 1.0F };
    const glm::vec3 direction { std::sin(angle), 0.0F, -std::cos(angle) };
    const Footprint footprint = kernel::surface::footprintOf(0.3F, direction, normal);
    BOOST_TEST(footprint.width() == 0.3F, boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(glm::length(footprint.major) == 1.2F, boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(std::fabs(footprint.major.x) == 1.2F, boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(std::fabs(footprint.minor.y) == 0.3F, boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(glm::dot(footprint.major, normal) == 0.0F);
}

// a rough bounce spreads the cone more than a smooth one, a convex mirror spreads it and a concave
// one focuses it, and a transmission keeps the spread it arrived with
BOOST_AUTO_TEST_CASE(moya_cone_a_rough_bounce_spreads_wider_than_a_smooth_one_test) {
    const Cone arriving { .width = 0.0F, .spread = 0.01F };
    const float width = 0.1F;
    const Cone smooth = kernel::surface::bounced(arriving, width, 0.0F, 0.0F, false);
    const Cone glossy = kernel::surface::bounced(arriving, width, 0.0F, 0.2F, false);
    const Cone rough = kernel::surface::bounced(arriving, width, 0.0F, 1.0F, false);
    BOOST_TEST(smooth.width == width);
    BOOST_TEST(smooth.spread == arriving.spread);
    BOOST_TEST(glossy.spread > smooth.spread);
    BOOST_TEST(rough.spread > glossy.spread);
    BOOST_TEST(rough.spread == arriving.spread + std::atan(1.0F), boost::test_tools::tolerance(1.0e-6F));

    BOOST_TEST(kernel::surface::bounced(arriving, width, 2.0F, 0.0F, false).spread == arriving.spread + 0.4F, boost::test_tools::tolerance(1.0e-6F));
    BOOST_TEST(kernel::surface::bounced(arriving, width, -2.0F, 0.0F, false).spread < arriving.spread);
    BOOST_TEST(kernel::surface::bounced(arriving, width, 2.0F, 0.0F, true).spread == arriving.spread);
}

// a lobe's roughness is its GGX alpha, the geometric mean of two, one for a diffuse lobe, and
// zero for a lobe too smooth to have microfacets
BOOST_AUTO_TEST_CASE(moya_cone_a_lobe_gives_its_roughness_test) {
    kernel::bsdf::Closure conductor;
    conductor.model = kernel::bsdf::Model::Conductor;
    conductor.alpha = glm::vec2 { 0.04F, 0.09F };
    BOOST_TEST(conductor.spread() == 0.06F, boost::test_tools::tolerance(1.0e-5F));
    conductor.alpha = glm::vec2 { 0.0F };
    BOOST_TEST(conductor.spread() == 0.0F);

    kernel::bsdf::Closure diffuse;
    diffuse.model = kernel::bsdf::Model::Diffuse;
    BOOST_TEST(diffuse.spread() == 1.0F);
}

};  // namespace
