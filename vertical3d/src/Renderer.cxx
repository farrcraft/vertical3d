/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Renderer.h"

#include <api/asset/Manager.h>
#include <api/asset/Type.h>
#include <api/asset/media/kind/Image.h>
#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/render/realtime/Frame.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/render/realtime/renderer/Line.h>
#include <api/shell/Shell.h>
#include <api/ui/Engine.h>
#include <api/ui/paint/Dressing.h>
#include <api/ui/paint/Text.h>
#include <api/ui/shell/Screen.h>
#include <api/window/Window.h>
#include <vertical3d/src/manipulator/Manipulator.h>
#include <vertical3d/src/manipulator/Target.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/tool/SelectTool.h>
#include <vertical3d/src/tool/TransformTool.h>
#include <vertical3d/src/view/ShadedPass.h>
#include <vertical3d/src/view/ViewPort.h>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/common.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

namespace {

/**
 * What every viewport clears to.
 **/
constexpr glm::vec4 background(0.16F, 0.17F, 0.19F, 1.0F);

/**
 * The outline of a rubber band.
 **/
constexpr glm::vec4 bandColour(0.9F, 0.9F, 0.9F, 1.0F);

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
 * Every metric below is a multiple of the text size, and the text size is a multiple of
 * this, so the editor scales by one number. Glyphs are distance fields, so a new scale
 * does not need a rebuilt atlas.
 **/
const float uiScale = 1.0F;

/**
 * The size the ui text is drawn at, and what the bar and its rows are sized from.
 **/
const float fontSize = 15.0F * uiScale;

/**
 * The ui's spacing, each a multiple of the text size.
 **/
constexpr float lineHeightScale = 1.5F;
constexpr float paddingScale = 1.4F;
constexpr float barHeightScale = 1.8F;
constexpr float panelPaddingScale = 0.3F;

};  // namespace

/**
 **/
Renderer::Renderer(const boost::shared_ptr<v3d::window::Window>& window,
    const boost::shared_ptr<v3d::log::Logger>& logger,
    const boost::shared_ptr<v3d::asset::Manager>& assetManager, v3d::shell::Shell* shell) :
    logger_(logger),
    assetManager_(assetManager),
    engine_(logger),
    shell_(shell),
    background_(background) {
    engine_.initialize(window);
    engine_.clearColour(background_);

    v3d::ui::shell::Screen::Options options;
    options.size = fontSize;
    options.dress = [](v3d::ui::paint::Dressing* style, float size) {
        style->lineHeight = size * lineHeightScale;
        style->padding = size * paddingScale;
        style->barHeight = size * barHeightScale;
        style->panelPadding = size * panelPaddingScale;
    };
    shell_->present(&engine_, options);
    screen_ = shell_->screen();

    shaded_ = boost::make_shared<ShadedPass>(engine_.context());
    if (!shaded_->nonSolid()) {
        logger_->get()->warn("the device cannot draw a triangle as its edges or its corners, so a view asked to draws it filled");
    }
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
void Renderer::transformTool(const boost::shared_ptr<TransformTool>& transform) {
    transform_ = transform;
}

/**
 **/
void Renderer::selectTool(const boost::shared_ptr<SelectTool>& select) {
    select_ = select;
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
        const boost::shared_ptr<v3d::asset::media::kind::Image> asset =
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
        return glm::vec2(0.0F, 0.0F);
    }
    return screen_->components().insets(*ui_);
}

/**
 **/
void Renderer::draw() {
    if (!screen_->begin()) {
        return;
    }
    const boost::shared_ptr<v3d::render::realtime::Frame> frame = engine_.frame();
    if (!frame) {
        return;
    }

    const boost::shared_ptr<v3d::render::realtime::renderer::Line> lines = engine_.lines();

    // a view with no scene still draws its grid: an empty document is not an error
    const Scene empty;
    const Scene& scene = scene_ ? *scene_ : empty;

    // the handles sit about what the tool would move, which every view draws the same
    const boost::shared_ptr<Manipulator> manipulator = transform_ ? transform_->manipulator() : boost::shared_ptr<Manipulator>();
    const Target target = transform_ ? transform_->target() : Target();

    for (std::size_t index = 0; index < views_.size(); index++) {
        const boost::shared_ptr<ViewPort>& view = views_[index];
        if (!view) {
            continue;
        }

        view->draw(scene, manipulator.get(), target, &canvases_[index], &overlays_[index]);

        const boost::shared_ptr<v3d::render::realtime::Pass> pass = frame->pass(view->name());
        pass->viewport(view->region());
        pass->clearColour(background_);
        // every view depth tests, so a wireframe is occluded by what is in front of it.
        // depth is cleared with colour, and each pass clears only its own region
        pass->depth(true);
        pass->camera(view->camera()->view(), view->camera()->projection());

        // the handles are drawn over what the scene pass left, without a depth test, so
        // a handle lying in the plane of the grid is not hidden by it
        const boost::shared_ptr<v3d::render::realtime::Pass> overlay = frame->pass(view->name() + handleSuffix);
        overlay->viewport(view->region());
        overlay->keepColour();
        overlay->depth(false);
        overlay->camera(view->camera()->view(), view->camera()->projection());

        // the surfaces first, so the wireframe drawn after them is drawn over them
        if (shaded_ && view->shows(ViewPort::SHOW_MESH)) {
            shaded_->submit(scene, view->display(), pass.get());
        }
        if (lines) {
            lines->submit(canvases_[index], pass.get());
            if (!overlays_[index].empty()) {
                lines->submit(overlays_[index], overlay.get());
            }
        }
    }

    drawUi(frame);

    engine_.renderFrame();
    if (shaded_) {
        shaded_->collect();
    }
}

/**
 **/
void Renderer::drawUi(const boost::shared_ptr<v3d::render::realtime::Frame>& frame) {
    // the statistics overlay draws onto the same canvas, so this runs even when there is no
    // ui to draw
    shell_->draw();
    v3d::render::realtime::canvas::Canvas& canvas = screen_->canvas();

    // the rubber band is drawn in window pixels, which is the space this canvas is in
    const std::optional<SelectTool::Band> band = select_ ? select_->band() : std::nullopt;
    if (band) {
        const glm::vec2 low = glm::min(band->corner, band->opposite);
        const glm::vec2 high = glm::max(band->corner, band->opposite);
        canvas.rect(low, glm::vec2(high.x, low.y + 1.0F), bandColour);
        canvas.rect(glm::vec2(low.x, high.y - 1.0F), high, bandColour);
        canvas.rect(low, glm::vec2(low.x + 1.0F, high.y), bandColour);
        canvas.rect(glm::vec2(high.x - 1.0F, low.y), high, bandColour);
    }

    if (canvas.empty()) {
        return;
    }

    // this pass covers the whole window, over what every view drew. A canvas carries its
    // own projection, so this pass needs no camera
    const boost::shared_ptr<v3d::render::realtime::Pass> pass = frame->pass(uiPass);
    pass->keepColour();
    pass->depth(false);
    engine_.quads()->submit(canvas, pass.get());
}

/**
 **/
v3d::render::realtime::TextureHandle Renderer::picture(const boost::shared_ptr<v3d::image::Image>& image) {
    // the ui stops drawing the old texture this frame, and its image is reclaimed once the
    // frame in flight has finished with it
    if (picture_.valid()) {
        engine_.textures()->release(picture_);
        picture_ = v3d::render::realtime::TextureHandle();
    }
    if (image) {
        picture_ = engine_.textures()->texture(image);
    }
    return picture_;
}

/**
 **/
void Renderer::shutdown() {
    // the surfaces' buffers go back to the ring, which destroys them once the device is idle
    shaded_.reset();
    engine_.shutdown();
}

};  // namespace v3d::editor
