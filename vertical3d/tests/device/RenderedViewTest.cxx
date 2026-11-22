/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <api/brep/Primitives.h>
#include <api/image/Image.h>
#include <api/image/reader/Png.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/render/realtime/canvas/Line.h>
#include <api/render/realtime/frame/Frame.h>
#include <api/render/realtime/frame/Pass.h>
#include <api/render/realtime/frame/RenderTarget.h>
#include <api/render/realtime/renderer/Line.h>
#include <api/render/realtime/renderer/Quad.h>
#include <api/render/realtime/resource/Handle.h>
#include <api/render/realtime/resource/Textures.h>
#include <api/render/realtime/vulkan/frame/Capture.h>
#include <api/render/tests/device/Headless.h>
#include <api/type/camera/Camera.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/scene/item/Mesh.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/view/RenderedViews.h>
#include <vertical3d/src/view/ShadedPass.h>
#include <vertical3d/src/view/Shading.h>
#include <vertical3d/src/view/ViewPort.h>

#include <cstddef>
#include <cstdint>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace {

const VkFormat colourFormat = VK_FORMAT_R8G8B8A8_UNORM;
const std::uint32_t size = 64;

/**
 * What the middle of a rendered view shows: a cube before the camera, drawn for its depth,
 * the picture over it, and a red line across the middle, behind the cube or before it.
 **/
std::string middleOf(v3d::test::Headless* headless, bool behind, const std::string& name) {
    using v3d::render::realtime::frame::RenderTarget;
    namespace canvas = v3d::render::realtime::canvas;

    v3d::type::camera::Profile profile("view");
    profile.orthographic(false);
    profile.fov(60.0F);
    profile.eye(glm::vec3(0.0F, 0.0F, -5.0F));
    profile.lookat(glm::vec3(0.0F));
    profile.size(glm::uvec2(size, size));
    profile.pixelAspect(1.0F);
    v3d::editor::view::ViewPort view("view", profile);
    view.resize(glm::vec4(0.0F, 0.0F, static_cast<float>(size), static_cast<float>(size)));
    view.display().shading = v3d::editor::view::Shading::Rendered;
    // the matrices are built as a view builds them when it draws
    view.camera()->createProjection();
    view.camera()->createView();

    v3d::editor::scene::Scene scene;
    scene.add(boost::make_shared<v3d::editor::scene::item::Mesh>(v3d::brep::cube()));

    // the picture is all blue
    const boost::shared_ptr<v3d::image::Image> image = boost::make_shared<v3d::image::Image>(glm::uvec2(4, 4), static_cast<std::uint8_t>(24));
    for (std::size_t texel = 0; texel < 16; texel++) {
        image->data()[(texel * 3) + 2] = 255;
    }
    const v3d::render::realtime::resource::TextureHandle picture = headless->context->textures()->texture(image);

    const boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(*headless->context,
        RenderTarget::Spec{ .width = size, .height = size, .colour = colourFormat, .depth = true });
    v3d::render::realtime::frame::Frame frame;
    const boost::shared_ptr<v3d::render::realtime::frame::Pass> pass = frame.pass("view");
    pass->target(target);
    pass->clearColour(glm::vec4(0.0F, 0.0F, 0.0F, 1.0F));
    pass->depth(true);
    pass->camera(view.camera()->view(), view.camera()->projection());

    v3d::editor::view::ShadedPass shaded(headless->context);
    shaded.submit(scene, { .shading = v3d::editor::view::Shading::Flat, .cull = v3d::editor::view::ViewPort::Cull::None }, pass.get());
    canvas::Canvas quad;
    v3d::editor::view::RenderedViews::cover(view, picture, &quad);
    headless->context->quads()->submit(quad, pass.get());
    canvas::Line lines;
    const float depth = behind ? 3.0F : -3.0F;
    lines.line(glm::vec3(-4.0F, 0.0F, depth), glm::vec3(4.0F, 0.0F, depth), glm::vec4(1.0F, 0.0F, 0.0F, 1.0F));
    headless->context->lines()->submit(lines, pass.get());

    v3d::render::realtime::vulkan::frame::Capture capture(headless->device, headless->context->ring(), headless->logger);
    headless->draw(frame, *target, &capture);
    shaded.collect();
    boost::filesystem::create_directory("data_out");
    const std::string path = "data_out/" + name + ".png";
    if (!capture.write(path)) {
        return "nothing written";
    }
    v3d::image::reader::Png png(headless->logger);
    const boost::shared_ptr<v3d::image::Image> read = png.read(path);
    if (!read) {
        return "nothing read";
    }
    // the line is a pixel wide, so the rows about the middle are searched for it
    bool line = false;
    bool blue = true;
    for (std::size_t row = (size / 2) - 2; row <= (size / 2) + 2; row++) {
        const unsigned char* at = read->data() + (((row * size) + (size / 2)) * 4);
        line = line || (at[0] > 200 && at[1] < 50 && at[2] < 50);
        blue = blue && at[0] < 50 && at[1] < 50 && at[2] > 200;
    }
    if (line) {
        return "line";
    }
    return blue ? "picture" : "neither";
}

BOOST_AUTO_TEST_SUITE(rendered_view_test)

/**
 * A line behind a surface is hidden by it in a rendered view, as in a shaded one, and the
 * picture shows there. A line before the surface is drawn over the picture.
 **/
BOOST_AUTO_TEST_CASE(a_line_behind_a_surface_is_hidden_under_the_picture) {
    v3d::test::Headless headless(colourFormat, size, size);
    BOOST_TEST(middleOf(&headless, true, "rendered_behind") == "picture");
    BOOST_TEST(middleOf(&headless, false, "rendered_before") == "line");
    BOOST_TEST(headless.silent());
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
