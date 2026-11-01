/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Renderer.h"

#include <api/asset/Manager.h>
#include <api/asset/media/kind/Image.h>
#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/render/realtime/Frame.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/render/realtime/renderer/Line.h>
#include <api/shell/Shell.h>
#include <api/ui/Image.h>
#include <api/ui/Immediate.h>
#include <api/ui/Interface.h>
#include <api/ui/paint/Dressing.h>
#include <api/ui/paint/Painter.h>
#include <api/ui/paint/Text.h>
#include <api/ui/shell/Screen.h>
#include <api/window/Window.h>
#include <vertical3d/src/manipulator/Manipulator.h>
#include <vertical3d/src/manipulator/Screen.h>
#include <vertical3d/src/manipulator/Target.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/selection/SelectMask.h>
#include <vertical3d/src/control/IconSheet.h>
#include <vertical3d/src/control/Notices.h>
#include <vertical3d/src/control/Pointer.h>
#include <vertical3d/src/control/StatusBar.h>
#include <vertical3d/src/tool/Select.h>
#include <vertical3d/src/tool/Transform.h>
#include <vertical3d/src/view/AreaFrame.h>
#include <vertical3d/src/view/AreaViews.h>
#include <vertical3d/src/view/ShadedPass.h>
#include <vertical3d/src/view/ViewOverlay.h>
#include <vertical3d/src/view/ViewPort.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/common.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

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
    options.immediate = true;
    options.dress = [](v3d::ui::paint::Dressing* style, float size) {
        style->lineHeight = size * lineHeightScale;
        style->padding = size * paddingScale;
        style->barHeight = size * barHeightScale;
        style->panelPadding = size * panelPaddingScale;
        style->iconSize = size * iconScale;
    };
    shell_->present(&engine_, options);
    screen_ = shell_->screen();

    shaded_ = boost::make_shared<view::ShadedPass>(engine_.context());
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
void Renderer::views(const boost::shared_ptr<view::AreaViews>& views) {
    views_ = views;
}

/**
 **/
void Renderer::pointer(const boost::shared_ptr<control::Pointer>& pointer) {
    pointer_ = pointer;
}

/**
 **/
void Renderer::status(const control::Status& status) {
    status_ = status;
}

/**
 **/
void Renderer::notices(const boost::shared_ptr<control::Notices>& notices) {
    notices_ = notices;
}

/**
 **/
void Renderer::panels(const Panels& draw) {
    panels_ = draw;
}

/**
 **/
v3d::ui::Immediate* Renderer::immediate() const noexcept {
    return screen_->immediate();
}

/**
 **/
void Renderer::scene(const boost::shared_ptr<scene::Scene>& scene) {
    scene_ = scene;
}

/**
 **/
void Renderer::transformTool(const boost::shared_ptr<tool::Transform>& transform) {
    transform_ = transform;
}

/**
 **/
void Renderer::selectTool(const boost::shared_ptr<tool::Select>& select) {
    select_ = select;
}

/**
 **/
v3d::ui::paint::Measure Renderer::measure() const {
    return screen_->text()->measure(screen_->size());
}

/**
 **/
void Renderer::ui(const boost::shared_ptr<v3d::ui::Interface>& ui) {
    ui_ = ui;
    if (!ui_) {
        return;
    }

    // a theme names its images but does not load them. The app loads each one as an
    // asset, and the device context's texture service owns the texture it becomes
    ui_->resolveImages([this](const std::string& source) { return image(source); });
}

/**
 **/
void Renderer::resolve(const boost::shared_ptr<v3d::ui::Component>& component) {
    if (ui_) {
        ui_->resolveComponentImages([this](const std::string& source) { return image(source); }, component);
    }
}

/**
 **/
void Renderer::icons(const control::IconSheet& sheet) {
    icons_ = sheet;
}

/**
 **/
v3d::ui::Image Renderer::image(const std::string& source) {
    if (const std::optional<v3d::ui::Image::Corners> corners = icons_.corners(source)) {
        const v3d::render::realtime::TextureHandle sheet = texture(icons_.image());
        return sheet.valid() ? v3d::ui::Image(sheet, *corners) : v3d::ui::Image();
    }
    return texture(source);
}

/**
 **/
v3d::render::realtime::TextureHandle Renderer::texture(const std::string& source) {
    const auto found = images_.find(source);
    if (found != images_.end()) {
        return found->second;
    }
    const boost::shared_ptr<v3d::asset::media::kind::Image> asset = assetManager_->load<v3d::asset::media::kind::Image>(source);
    if (!asset || !asset->image()) {
        return v3d::render::realtime::TextureHandle();
    }
    const v3d::render::realtime::TextureHandle made = engine_.textures()->texture(asset->image());
    images_[source] = made;
    return made;
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
float Renderer::scale() const noexcept {
    return screen_->factor();
}

/**
 **/
float Renderer::barHeight() const {
    return std::as_const(screen_->components()).dressing().barHeight;
}

/**
 **/
view::OverlayMetrics Renderer::overlay() const {
    return view::OverlayMetrics::scaled(screen_->factor(), barHeight());
}

/**
 **/
void Renderer::draw() {
    if (!screen_->begin(shell_->ui().get())) {
        return;
    }
    const boost::shared_ptr<v3d::render::realtime::Frame> frame = engine_.frame();
    if (!frame) {
        return;
    }

    const boost::shared_ptr<v3d::render::realtime::renderer::Line> lines = engine_.lines();

    // a view with no scene still draws its grid: an empty document is not an error
    const scene::Scene empty;
    const scene::Scene& scene = scene_ ? *scene_ : empty;

    // the handles sit about what the tool would move, which every view draws the same
    const boost::shared_ptr<manipulator::Manipulator> manipulator = transform_ ? transform_->manipulator() : boost::shared_ptr<manipulator::Manipulator>();
    const manipulator::Target target = transform_ ? transform_->target() : manipulator::Target();

    static const std::vector<boost::shared_ptr<view::ViewPort>> none;
    const std::vector<boost::shared_ptr<view::ViewPort>>& views = views_ ? views_->shown() : none;
    retire(frame.get(), views);
    canvases_.resize(views.size());
    overlays_.resize(views.size());
    for (std::size_t index = 0; index < views.size(); index++) {
        const boost::shared_ptr<view::ViewPort>& view = views[index];
        if (!view) {
            continue;
        }

        view->draw(scene, &canvases_[index], select_ ? select_->mask() : selection::SelectMask::Object);
        // the handles go to a canvas of their own because they are an overlay, drawn in a
        // second pass that does not depth test. On the scene's canvas a handle would be in
        // the depth tested pass, where the grid's axis lines lie in the same plane and hide it
        overlays_[index].clear();
        if (manipulator && view->shows(view::ViewPort::SHOW_HANDLE)) {
            manipulator->draw(target, manipulator::Screen{ .camera = view->camera(), .region = view->region() }, &overlays_[index]);
        }

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
        if (shaded_ && view->shows(view::ViewPort::SHOW_MESH)) {
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
    v3d::render::realtime::canvas::Canvas& canvas = screen_->canvas();
    // each view's axis marker and gizmo go under the frame, whose corners cover the views' own
    if (views_ && pointer_) {
        const float factor = screen_->factor();
        view::OverlayLook look;
        look.text *= factor;
        look.line = std::max(1.0F, std::round(look.line * factor));
        const view::OverlayMetrics metrics = overlay();
        const v3d::ui::paint::Measure measure = screen_->text()->measure(look.text, v3d::ui::paint::Weight::SemiBold);
        const v3d::ui::paint::Write write = screen_->text()->write(&canvas, look.text, v3d::ui::paint::Weight::SemiBold);
        const glm::vec2& cursor = pointer_->cursor();
        for (const boost::shared_ptr<view::ViewPort>& view : views_->shown()) {
            const glm::vec4& region = view->region();
            const bool over = cursor.x >= region.x && cursor.x < region.x + region.z && cursor.y >= region.y && cursor.y < region.y + region.w;
            drawOverlay(&canvas, *view, metrics, look, measure, write, over ? std::optional<glm::vec2>(cursor) : std::nullopt);
        }
    }
    // the panels fill their areas, and the frame's corners and lines go over them
    if (panels_) {
        panels_(screen_->immediate(), &canvas);
    }
    // the frame around the areas goes first, so a menu dropped over it covers it
    if (views_ && pointer_) {
        const v3d::ui::paint::Dressing& dress = std::as_const(screen_->components()).dressing();
        const float factor = screen_->factor();
        const view::FrameLook look{ .gap = dress.gap, .hairline = dress.hairline, .accent = dress.focus,
            .landing = glm::vec4(dress.focus.r, dress.focus.g, dress.focus.b, landingAlpha),
            .radius = view::FrameLook::RADIUS * factor, .line = std::max(1.0F, std::floor(factor)) };
        const std::optional<control::Pointer::Drop> landing = pointer_->drop();
        drawFrame(&canvas, *views_->layout(), look, pointer_->activeArea(), pointer_->lit(),
            landing ? std::optional<glm::vec4>(landing->region) : std::nullopt);
    }
    drawNotice(&canvas);
    drawStatus(&canvas);
    // the statistics overlay draws onto the same canvas, so this runs even when there is no
    // ui to draw
    shell_->draw();

    // the rubber band is drawn in window pixels, which is the space this canvas is in
    const std::optional<tool::Select::Band> band = select_ ? select_->band() : std::nullopt;
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
void Renderer::retire(v3d::render::realtime::Frame* frame, const std::vector<boost::shared_ptr<view::ViewPort>>& views) {
    std::vector<std::string> names;
    for (const boost::shared_ptr<view::ViewPort>& view : views) {
        if (view) {
            names.push_back(view->name());
        }
    }
    if (names == viewPasses_) {
        return;
    }
    bool added = false;
    for (const std::string& name : names) {
        added = added || std::ranges::find(viewPasses_, name) == viewPasses_.end();
    }
    for (const std::string& name : viewPasses_) {
        if (std::ranges::find(names, name) == names.end()) {
            frame->remove(name);
            frame->remove(name + handleSuffix);
        }
    }
    if (added) {
        frame->remove(uiPass);
    }
    viewPasses_ = std::move(names);
}

/**
 **/
void Renderer::drawStatus(v3d::render::realtime::canvas::Canvas* canvas) const {
    const v3d::ui::paint::Dressing& dress = std::as_const(screen_->components()).dressing();
    // a toolbar down the left runs the height of the window, so the bar starts beside it
    const float left = insets().x;
    const float width = static_cast<float>(canvas->width());
    const float bottom = static_cast<float>(canvas->height());
    const float top = bottom - dress.barHeight;
    canvas->rect(glm::vec2(left, top), glm::vec2(width, bottom), dress.bar);
    canvas->rect(glm::vec2(left, top), glm::vec2(width, top + v3d::ui::paint::Dressing::outline(1.0F, screen_->factor())), dress.hairline);

    const float size = screen_->size();
    const v3d::ui::paint::Measure measure = screen_->text()->measure(size);
    const v3d::ui::paint::Write write = screen_->text()->write(canvas, size);
    const float baseline = top + (dress.barHeight / 2.0F) + (size * BASELINE);
    // a button's name, then what it does, then a wider gap before the next button
    float x = left + dress.padding;
    for (const control::Status::Button& button : status_.buttons) {
        write(button.name, glm::vec2(x, baseline), dress.secondaryText);
        x += measure(button.name) + (size / 2.0F);
        write(button.action, glm::vec2(x, baseline), dress.text);
        x += measure(button.action) + dress.padding;
    }

    std::string right;
    for (const std::string* part : { &status_.selection, &status_.frame, &status_.rate }) {
        if (!part->empty()) {
            right += (right.empty() ? "" : STATUS_GAP) + *part;
        }
    }
    write(right, glm::vec2(width - dress.padding - measure(right), baseline), dress.secondaryText);
}

/**
 **/
void Renderer::drawNotice(v3d::render::realtime::canvas::Canvas* canvas) {
    if (!notices_ || !views_ || !pointer_ || views_->layout()->areas().empty()) {
        return;
    }
    const std::optional<control::Notices::Shown> shown = notices_->shown(screen_->clock().now);
    const glm::vec4& content = views_->layout()->areas()[pointer_->activeArea()].content;
    if (!shown || content.z <= 0.0F || content.w <= 0.0F) {
        return;
    }
    const v3d::ui::paint::Dressing& dress = std::as_const(screen_->components()).dressing();
    const float size = screen_->size();
    const v3d::ui::paint::Measure measure = screen_->text()->measure(size);
    const v3d::ui::paint::Write write = screen_->text()->write(canvas, size);

    const glm::vec2 plate(measure(shown->text) + (dress.padding * 2.0F), dress.barHeight);
    // centred on the area, and moved in from the edge of the areas when the area is narrower
    // than the plate
    const float margin = NOTICE_MARGIN * screen_->factor();
    const glm::vec4& room = views_->layout()->region();
    const float centred = content.x + ((content.z - plate.x) / 2.0F);
    const float left = std::max(room.x + margin, std::min(centred, room.x + room.z - margin - plate.x));
    const glm::vec2 min(std::round(left), std::round(content.y + content.w - margin - plate.y));
    const glm::vec2 max = min + plate;
    canvas->fade(shown->alpha);
    const glm::vec2 drop(0.0F, dress.shadowOffset);
    v3d::ui::paint::shadowBox(canvas, min + drop, max + drop, { .radius = dress.radius, .reach = dress.shadowSize, .colour = dress.shadow });
    v3d::ui::paint::plateBox(canvas, min, max, dress.radius, dress.borderWidth, { .inside = dress.panel, .outline = dress.border });
    write(shown->text, glm::vec2(min.x + dress.padding, min.y + (plate.y / 2.0F) + (size * BASELINE)), dress.text);
    canvas->unfade();
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
