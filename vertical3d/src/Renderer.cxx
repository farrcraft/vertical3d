/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Renderer.h"

#include <api/asset/media/kind/Image.h>
#include <api/asset/Type.h>
#include <api/render/realtime/Frame.h>
#include <api/render/realtime/Pass.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::editor {

namespace {

/**
 * What every viewport clears to.
 **/
constexpr glm::vec4 background(0.16f, 0.17f, 0.19f, 1.0f);

/**
 * What a view's handle pass is called, on the end of the view's own name.
 **/
const char* const handleSuffix = " handles";

/**
 * The pass that belongs to no view, drawn over all of them.
 **/
const char* const uiPass = "ui";

/**
 * How large the whole ui is drawn, as a multiple of the size it was laid out at.
 *
 * Every metric below is a multiple of the text size and the text size is a multiple of
 * this, so the editor scales by one number. Glyphs are distance fields, so a new scale
 * does not need a rebuilt atlas.
 **/
const float uiScale = 1.0f;

/**
 * The size the ui text is drawn at, and what the bar and its rows are sized from.
 **/
const float fontSize = 15.0f * uiScale;

};  // namespace

/**
 **/
Renderer::Renderer(const boost::shared_ptr<v3d::render::realtime::Window>& window,
    const boost::shared_ptr<v3d::log::Logger>& logger,
    const boost::shared_ptr<v3d::asset::Manager>& assetManager) :
    logger_(logger),
    assetManager_(assetManager),
    engine_(logger, assetManager),
    background_(background) {
    engine_.initialize(window);
    engine_.clearColour(background_);

    v3d::ui::shell::Screen::Options options;
    options.size = fontSize;
    options.dress = [](v3d::ui::paint::Dressing* style, float size) {
        style->lineHeight = size * 1.5f;
        style->padding = size * 1.4f;
        style->barHeight = size * 1.8f;
        style->panelPadding = size * 0.3f;
    };
    screen_ = boost::make_shared<v3d::ui::shell::Screen>(&engine_, assetManager, logger, options);
}

/**
 **/
Renderer::~Renderer() {
}

/**
 **/
void Renderer::views(const std::vector<boost::shared_ptr<ViewPort>>& views) {
    views_ = views;
    canvases_.resize(views_.size());
    overlays_.resize(views_.size());
}

/**
 **/
void Renderer::scene(const boost::shared_ptr<Scene>& scene) {
    scene_ = scene;
}

/**
 **/
void Renderer::manipulator(const boost::shared_ptr<Manipulator>& manipulator) {
    manipulator_ = manipulator;
}

/**
 **/
const boost::shared_ptr<v3d::ui::shell::StatisticsOverlay>& Renderer::statistics() const {
    return screen_->statistics();
}

/**
 **/
v3d::ui::paint::Measure Renderer::measure() const {
    return screen_->text()->measure(screen_->size());
}

/**
 **/
void Renderer::ui(const boost::shared_ptr<v3d::ui::Engine>& ui) {
    ui_ = ui;
    if (!ui_) {
        return;
    }

    // a theme names its images but does not load them. The app loads each one as an
    // asset, and the device context's texture service owns the texture it becomes
    ui_->resolveImages([this](const std::string& source) -> v3d::render::realtime::TextureHandle {
        const v3d::asset::Type type = source.ends_with(".png")
            ? v3d::asset::Type::ImagePng : v3d::asset::Type::ImageTga;
        boost::shared_ptr<v3d::asset::media::kind::Image> asset =
            assetManager_->load<v3d::asset::media::kind::Image>(source, type);
        if (!asset || !asset->image()) {
            return v3d::render::realtime::TextureHandle();
        }
        return engine_.textures()->texture(asset->image());
    });

    // the metrics the constructor worked out from the font size stand unless the theme
    // names its own
    screen_->theme(ui_->activeTheme());
}

/**
 **/
glm::vec2 Renderer::insets() const {
    if (!ui_) {
        return glm::vec2(0.0f, 0.0f);
    }
    return screen_->components().insets(*ui_);
}

/**
 **/
void Renderer::draw(const v3d::ui::shell::StatisticsOverlay::Sample& statistics) {
    if (!screen_->begin()) {
        return;
    }
    boost::shared_ptr<v3d::render::realtime::Frame> frame = engine_.frame();
    if (!frame) {
        return;
    }

    boost::shared_ptr<v3d::render::realtime::vulkan::renderer::Line> lines = engine_.lines();

    // a view with no scene still draws its grid: an empty document is not an error
    const Scene empty;
    const Scene& scene = scene_ ? *scene_ : empty;

    for (std::size_t index = 0; index < views_.size(); index++) {
        const boost::shared_ptr<ViewPort>& view = views_[index];
        if (!view) {
            continue;
        }

        view->draw(scene, manipulator_.get(), &canvases_[index], &overlays_[index]);

        boost::shared_ptr<v3d::render::realtime::Pass> pass = frame->pass(view->name());
        pass->viewport(view->region());
        pass->clearColour(background_);
        // every view depth tests, so a wireframe is occluded by what is in front of it.
        // depth is cleared with colour, and each pass clears only its own region
        pass->depth(true);
        pass->camera(view->camera()->view(), view->camera()->projection());

        // the handles are drawn over what the scene pass left, without a depth test, so
        // a handle lying in the plane of the grid is not hidden by it
        boost::shared_ptr<v3d::render::realtime::Pass> overlay = frame->pass(view->name() + handleSuffix);
        overlay->viewport(view->region());
        overlay->keepColour();
        overlay->depth(false);
        overlay->camera(view->camera()->view(), view->camera()->projection());

        if (lines) {
            lines->submit(canvases_[index], pass.get());
            if (!overlays_[index].empty()) {
                lines->submit(overlays_[index], overlay.get());
            }
        }
    }

    drawUi(frame, statistics);

    engine_.renderFrame();
}

/**
 **/
void Renderer::drawUi(const boost::shared_ptr<v3d::render::realtime::Frame>& frame,
    const v3d::ui::shell::StatisticsOverlay::Sample& statistics) {
    // the overlay draws onto the same canvas, so this runs for it whether or not there is
    // a ui to draw as well
    screen_->draw(ui_.get(), statistics);
    v3d::render::realtime::Canvas& canvas = screen_->canvas();
    if (canvas.empty()) {
        return;
    }

    // over the whole window rather than a view, and over what every view left. A canvas
    // carries its own projection, so this pass needs no camera
    boost::shared_ptr<v3d::render::realtime::Pass> pass = frame->pass(uiPass);
    pass->keepColour();
    pass->depth(false);
    engine_.quads()->submit(canvas, pass.get());
}

/**
 **/
void Renderer::shutdown() {
    engine_.shutdown();
}

};  // namespace v3d::editor
