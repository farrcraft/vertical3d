/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/BRep.h>
#include <api/brep/Face.h>
#include <api/brep/Index.h>
#include <api/brep/Topology.h>
#include <api/brep/Vertex.h>
#include <api/image/Image.h>
#include <api/image/reader/Png.h>
#include <api/render/realtime/Frame.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/vulkan/frame/Capture.h>
#include <api/render/realtime/vulkan/frame/Recorder.h>
#include <api/render/realtime/vulkan/frame/RenderTarget.h>
#include <api/render/realtime/vulkan/frame/Ring.h>
#include <api/render/tests/device/Headless.h>
#include <api/type/camera/Camera.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/scene/CreatePoly.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/view/ShadedPass.h>
#include <vertical3d/src/view/Triangulation.h>
#include <vertical3d/src/view/ViewPort.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace {

const VkFormat colourFormat = VK_FORMAT_R8G8B8A8_UNORM;
const uint32_t size = 64;
constexpr glm::vec4 background(0.0F, 0.0F, 0.0F, 1.0F);

/**
 * A perspective camera at z of -5 looking along +z at the origin, as the editor's views are
 * made: through a camera profile, so the case draws with the editor's own conventions.
 **/
v3d::type::camera::Camera camera() {
    v3d::type::camera::Profile profile("view");
    profile.orthographic(false);
    profile.fov(60.0F);
    profile.eye(glm::vec3(0.0F, 0.0F, -5.0F));
    profile.lookat(glm::vec3(0.0F));
    profile.size(glm::uvec2(size, size));
    profile.pixelAspect(1.0F);
    v3d::type::camera::Camera result(profile);
    result.createProjection();
    result.createView();
    return result;
}

/**
 * One face of a cube, the one facing the camera, as a mesh of its own: its outside faces the
 * camera, and its back faces away.
 **/
boost::shared_ptr<v3d::brep::BRep> facing() {
    const boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    for (v3d::brep::Index face = 0; face < cube->faceCount(); face++) {
        if (cube->face(face)->normal().z < -0.5F) {
            std::vector<glm::vec3> points;
            for (const v3d::brep::Index edge : v3d::brep::faceLoop(*cube, face)) {
                points.push_back(cube->vertex(cube->edge(edge)->vertex())->point());
            }
            const boost::shared_ptr<v3d::brep::BRep> quad = boost::make_shared<v3d::brep::BRep>();
            quad->addFace(points, cube->face(face)->normal());
            return quad;
        }
    }
    return nullptr;
}

/**
 * Draw a scene through the shaded pass into a fresh target, as a view of that display would,
 * and read back the pixel at the middle.
 **/
glm::vec4 middle(v3d::test::Headless* headless, v3d::editor::ShadedPass* shaded, const v3d::editor::Scene& scene,
    const v3d::editor::ViewPort::Display& display, const std::string& name) {
    using v3d::render::realtime::vulkan::frame::Capture;
    using v3d::render::realtime::vulkan::frame::Recorder;
    using v3d::render::realtime::vulkan::frame::RenderTarget;

    const boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(
        headless->device, headless->context->ring(), size, size, colourFormat, true);
    const v3d::type::camera::Camera view = camera();

    v3d::render::realtime::Frame frame;
    const boost::shared_ptr<v3d::render::realtime::Pass> pass = frame.pass("view");
    pass->clearColour(background);
    pass->depth(true);
    pass->camera(view.view(), view.projection());
    shaded->submit(scene, display, pass.get());

    Capture capture(headless->device, headless->logger);
    VkCommandBuffer commands = headless->context->ring()->begin();
    Recorder::Target described;
    described.image = target->image();
    described.view = target->view();
    described.extent = target->extent();
    described.depthImage = target->depthImage();
    described.depthView = target->depthView();
    described.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    Recorder::record(commands, frame, described, *headless->context->resources(), headless->context->frameUniforms().get());

    Capture::Source source;
    source.image = target->image();
    source.extent = target->extent();
    source.format = target->format();
    source.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    capture.record(commands, source);
    headless->submitAndWait(commands);
    BOOST_CHECK(headless->silent());
    shaded->collect();

    const std::string path = "data_out/" + name + ".png";
    BOOST_REQUIRE(capture.write(path));
    v3d::image::reader::Png png(headless->logger);
    const boost::shared_ptr<v3d::image::Image> picture = png.read(path);
    BOOST_REQUIRE(picture != nullptr);
    const std::size_t centre = ((static_cast<std::size_t>(size / 2) * size) + (size / 2)) * 4;
    const unsigned char* texel = picture->data() + centre;
    return glm::vec4(texel[0], texel[1], texel[2], texel[3]) / 255.0F;
}

bool lit(const glm::vec4& colour) {
    return glm::length(glm::vec3(colour)) > 0.1F;
}

BOOST_AUTO_TEST_SUITE(shaded_pass_test)

/**
 * A face whose outside faces the view is drawn when the back is culled and when nothing is,
 * and not when the front is culled. The case pins which winding the rasterizer counts as a
 * front face under the editor's camera.
 **/
BOOST_AUTO_TEST_CASE(a_face_toward_the_view_is_its_front) {
    v3d::test::Headless headless(colourFormat, size, size);
    v3d::editor::ShadedPass shaded(headless.context);
    v3d::editor::Scene scene;
    scene.add(boost::make_shared<v3d::editor::Mesh>(facing()));

    v3d::editor::ViewPort::Display display;
    display.cull = v3d::editor::ViewPort::Cull::Back;
    const glm::vec4 front = middle(&headless, &shaded, scene, display, "shaded_cull_back");
    BOOST_CHECK(lit(front));

    // lit head on by the light at the eye, so the surface comes out at its own colour
    BOOST_CHECK_CLOSE(front.r, v3d::editor::ShadedPass::surface.r, 2.0F);

    display.cull = v3d::editor::ViewPort::Cull::Front;
    BOOST_CHECK(!lit(middle(&headless, &shaded, scene, display, "shaded_cull_front")));

    display.cull = v3d::editor::ViewPort::Cull::None;
    BOOST_CHECK(lit(middle(&headless, &shaded, scene, display, "shaded_cull_none")));

    display.cull = v3d::editor::ViewPort::Cull::Both;
    BOOST_CHECK(!lit(middle(&headless, &shaded, scene, display, "shaded_cull_both")));
}

/**
 * Drawn as its edges or its corners, a face leaves its middle bare, where the device can draw
 * that way. Each fill and each shading draws without a validation message.
 **/
BOOST_AUTO_TEST_CASE(every_fill_and_shading_draws_cleanly) {
    v3d::test::Headless headless(colourFormat, size, size);
    v3d::editor::ShadedPass shaded(headless.context);
    v3d::editor::Scene scene;
    scene.add(boost::make_shared<v3d::editor::Mesh>(facing()));

    for (const v3d::editor::Shading shading : { v3d::editor::Shading::Flat, v3d::editor::Shading::Smooth }) {
        for (const v3d::editor::ViewPort::Fill fill :
            { v3d::editor::ViewPort::Fill::Points, v3d::editor::ViewPort::Fill::Lines, v3d::editor::ViewPort::Fill::Fill }) {
            v3d::editor::ViewPort::Display display;
            display.shading = shading;
            display.cull = v3d::editor::ViewPort::Cull::None;
            display.front = fill;
            display.back = fill;
            const glm::vec4 centre = middle(&headless, &shaded, scene, display, "shaded_fill");
            if (fill == v3d::editor::ViewPort::Fill::Fill || !shaded.nonSolid()) {
                BOOST_CHECK(lit(centre));
            } else {
                BOOST_CHECK(!lit(centre));
            }
        }
    }
}

/**
 * A mesh is uploaded once for each shading it is drawn in, again only when its geometry
 * changes, and let go of once nothing draws it.
 **/
BOOST_AUTO_TEST_CASE(a_mesh_is_uploaded_again_only_when_it_changes) {
    v3d::test::Headless headless(colourFormat, size, size);
    v3d::editor::ShadedPass shaded(headless.context);
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::editor::Mesh> item = boost::make_shared<v3d::editor::Mesh>(facing());
    scene.add(item);
    const v3d::editor::ViewPort::Display display;

    middle(&headless, &shaded, scene, display, "shaded_upload");
    middle(&headless, &shaded, scene, display, "shaded_upload");
    BOOST_CHECK_EQUAL(shaded.built(), 1U);

    // moving the item uploads nothing: its placement reaches the shader at each draw
    item->translation(glm::vec3(0.25F, 0.0F, 0.0F));
    middle(&headless, &shaded, scene, display, "shaded_upload");
    BOOST_CHECK_EQUAL(shaded.built(), 1U);

    // a change to the geometry does, and so does another shading
    item->geometry()->vertex(0)->point(item->geometry()->vertex(0)->point() * 1.1F);
    item->geometry()->touch();
    middle(&headless, &shaded, scene, display, "shaded_upload");
    BOOST_CHECK_EQUAL(shaded.built(), 2U);
    v3d::editor::ViewPort::Display smooth;
    smooth.shading = v3d::editor::Shading::Smooth;
    middle(&headless, &shaded, scene, smooth, "shaded_upload");
    BOOST_CHECK_EQUAL(shaded.built(), 3U);

    // a frame that draws none of them lets them all go
    BOOST_CHECK_EQUAL(shaded.uploaded(), 1U);
    shaded.collect();
    BOOST_CHECK_EQUAL(shaded.uploaded(), 0U);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
