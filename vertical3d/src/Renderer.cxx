/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Renderer.h"

#include <api/asset/File.h>
#include <api/asset/Manager.h>
#include <api/event/Event.h>
#include <api/asset/media/kind/Image.h>
#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/render/realtime/frame/Frame.h>
#include <api/render/realtime/resource/Handle.h>
#include <api/render/realtime/frame/Pass.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/render/realtime/renderer/Line.h>
#include <api/shell/Shell.h>
#include <api/ui/Image.h>
#include <api/ui/Immediate.h>
#include <api/ui/Interface.h>
#include <api/ui/paint/Dressing.h>
#include <api/ui/paint/Painter.h>
#include <api/ui/paint/Text.h>
#include <api/ui/paint/Type.h>
#include <api/ui/shell/Screen.h>
#include <api/window/Window.h>
#include <vertical3d/src/manipulator/Manipulator.h>
#include <vertical3d/src/manipulator/Screen.h>
#include <vertical3d/src/manipulator/Target.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/selection/SelectMask.h>
#include <vertical3d/src/control/chrome/DividerHint.h>
#include <vertical3d/src/control/IconSheet.h>
#include <vertical3d/src/control/chrome/Notices.h>
#include <vertical3d/src/control/chrome/Pointer.h>
#include <vertical3d/src/control/chrome/StatusBar.h>
#include <vertical3d/src/tool/Select.h>
#include <vertical3d/src/tool/Transform.h>
#include <vertical3d/src/view/AreaFrame.h>
#include <vertical3d/src/view/AreaViews.h>
#include <vertical3d/src/view/ConstructionPlane.h>
#include <vertical3d/src/view/ShadedPass.h>
#include <vertical3d/src/view/ViewOverlay.h>
#include <vertical3d/src/view/ViewPort.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/common.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
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
void Renderer::opened(const Opened& draw) {
    opened_ = draw;
}

/**
 **/
bool Renderer::open(const boost::shared_ptr<v3d::window::Window>& window) {
    const boost::shared_ptr<v3d::render::realtime::WindowRenderer::Output> output = engine_.open(window);
    if (!output) {
        return false;
    }
    const boost::shared_ptr<Other> other = boost::make_shared<Other>();
    other->window = window;
    other->output = output;
    other->layer = boost::make_shared<v3d::ui::Immediate>(screen_->text(), screen_->size());
    others_.push_back(other);
    return true;
}

/**
 **/
void Renderer::close(const boost::shared_ptr<v3d::window::Window>& window) {
    const auto found = std::ranges::find(others_, window, &Other::window);
    if (found == others_.end()) {
        return;
    }
    engine_.close((*found)->output);
    others_.erase(found);
}

/**
 **/
void Renderer::drawOthers() {
    v3d::ui::Immediate* main = screen_->immediate();
    const v3d::ui::paint::Dressing& dress = std::as_const(screen_->components()).dressing();
    for (const boost::shared_ptr<Other>& other : others_) {
        const glm::uvec2 size(static_cast<unsigned int>(std::max(0, other->window->width())),
            static_cast<unsigned int>(std::max(0, other->window->height())));
        if (size.x == 0 || size.y == 0) {
            continue;
        }
        if (other->canvas.width() != size.x || other->canvas.height() != size.y) {
            other->canvas.resize(size);
        }
        other->canvas.clear();
        // the layer follows the main one's scale, theme and clock, so both windows look alike
        other->layer->scale(screen_->factor());
        other->layer->dressing() = main->dressing();
        other->layer->clock(screen_->clock());
        if (opened_) {
            opened_(other->layer.get(), &other->canvas, other->window->id());
        }
        const boost::shared_ptr<v3d::render::realtime::frame::Pass> pass =
            other->output->frame()->pass(v3d::render::realtime::WindowRenderer::colourPass);
        pass->clearColour(dress.panel);
        pass->depth(false);
        if (!other->canvas.empty()) {
            engine_.quads()->submit(other->canvas, pass.get());
        }
    }
}

/**
 **/
void Renderer::views(const boost::shared_ptr<view::AreaViews>& views) {
    views_ = views;
}

/**
 **/
void Renderer::pointer(const boost::shared_ptr<control::chrome::Pointer>& pointer) {
    pointer_ = pointer;
}

/**
 **/
void Renderer::status(const control::chrome::Status& status) {
    status_ = status;
}

/**
 **/
void Renderer::notices(const boost::shared_ptr<control::chrome::Notices>& notices) {
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
v3d::ui::paint::Type Renderer::type() const {
    return v3d::ui::paint::Type(screen_->text(), screen_->size());
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
    iconSide_ = 0;
    drawIcons();
}

/**
 **/
v3d::ui::Image Renderer::image(const std::string& source) {
    if (icons_.corners(source)) {
        return icon(source);
    }
    return texture(source);
}

/**
 **/
void Renderer::drawIcons() {
    const v3d::ui::paint::Dressing& dress = std::as_const(screen_->components()).dressing();
    // a theme's size is read from a file, so it may be no size an icon can be drawn at
    if (!(dress.iconSize >= 1.0F) || !(dress.iconSize <= LARGEST_ICON)) {
        return;
    }
    const auto side = static_cast<unsigned int>(std::ceil(dress.iconSize));  // checked: in [1, LARGEST_ICON]
    if (side == iconSide_ || icons_.names().empty()) {
        return;
    }
    iconSide_ = side;
    const boost::shared_ptr<v3d::image::Image> sheet = icons_.draw(side,
        [this](const std::string& path) { return v3d::asset::readFile(assetManager_->path(path)); }, logger_);
    // the old texture is reclaimed once the frame in flight has finished with it
    if (iconTexture_.valid()) {
        engine_.textures()->release(iconTexture_);
        iconTexture_ = v3d::render::realtime::resource::TextureHandle();
    }
    if (sheet) {
        iconTexture_ = engine_.textures()->texture(sheet);
    }
    if (ui_) {
        ui_->resolveImages([this](const std::string& source) { return image(source); });
    }
}

/**
 **/
v3d::render::realtime::resource::TextureHandle Renderer::texture(const std::string& source) {
    const auto found = images_.find(source);
    if (found != images_.end()) {
        return found->second;
    }
    const boost::shared_ptr<v3d::asset::media::kind::Image> asset = assetManager_->load<v3d::asset::media::kind::Image>(source);
    if (!asset || !asset->image()) {
        return v3d::render::realtime::resource::TextureHandle();
    }
    const v3d::render::realtime::resource::TextureHandle made = engine_.textures()->texture(asset->image());
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
    // a change of scale changes the icon size, and the icons are drawn again at the new one
    drawIcons();
    const boost::shared_ptr<v3d::render::realtime::frame::Frame> frame = engine_.frame();
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
    const view::ConstructionPlane::Colours grid = gridColours();
    canvases_.resize(views.size());
    overlays_.resize(views.size());
    for (std::size_t index = 0; index < views.size(); index++) {
        const boost::shared_ptr<view::ViewPort>& view = views[index];
        if (!view) {
            continue;
        }

        view->grid().colours(grid);
        view->draw(scene, &canvases_[index], select_ ? select_->mask() : selection::SelectMask::Object);
        // the handles go to a canvas of their own because they are an overlay, drawn in a
        // second pass that does not depth test. On the scene's canvas a handle would be in
        // the depth tested pass, where the grid's axis lines lie in the same plane and hide it
        overlays_[index].clear();
        if (manipulator && view->shows(view::ViewPort::SHOW_HANDLE)) {
            manipulator->draw(target, manipulator::Screen{ .camera = view->camera(), .region = view->region() }, &overlays_[index]);
        }

        const boost::shared_ptr<v3d::render::realtime::frame::Pass> pass = frame->pass(view->name());
        pass->viewport(view->region());
        pass->clearColour(background_);
        // every view depth tests, so a wireframe is occluded by what is in front of it.
        // depth is cleared with colour, and each pass clears only its own region
        pass->depth(true);
        pass->camera(view->camera()->view(), view->camera()->projection());

        // the handles are drawn over what the scene pass left, without a depth test, so
        // a handle lying in the plane of the grid is not hidden by it
        const boost::shared_ptr<v3d::render::realtime::frame::Pass> overlay = frame->pass(view->name() + handleSuffix);
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
    drawOthers();

    engine_.renderFrame();
    if (shaded_) {
        shaded_->collect();
    }
}

/**
 **/
void Renderer::drawUi(const boost::shared_ptr<v3d::render::realtime::frame::Frame>& frame) {
    v3d::render::realtime::canvas::Canvas& canvas = screen_->canvas();
    // each view's axis marker and gizmo go under the frame, whose corners cover the views' own
    if (views_ && pointer_) {
        const float factor = screen_->factor();
        view::OverlayLook look;
        look.text *= factor;
        look.line = std::max(1.0F, std::round(look.line * factor));
        const view::OverlayMetrics metrics = overlay();
        const v3d::ui::paint::Type type(screen_->text(), look.text, v3d::ui::paint::Weight::SemiBold);
        const glm::vec2& cursor = pointer_->cursor();
        for (const boost::shared_ptr<view::ViewPort>& view : views_->shown()) {
            const glm::vec4& region = view->region();
            const bool over = cursor.x >= region.x && cursor.x < region.x + region.z && cursor.y >= region.y && cursor.y < region.y + region.w;
            drawOverlay(&canvas, *view, metrics, look, type, over ? std::optional<glm::vec2>(cursor) : std::nullopt);
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
        const std::optional<control::chrome::Pointer::Drop> landing = pointer_->drop();
        drawFrame(&canvas, *views_->layout(), look, pointer_->activeArea(), pointer_->lit(),
            landing ? std::optional<glm::vec4>(landing->region) : std::nullopt);
    }
    drawNotice(&canvas);
    drawDividerHint(&canvas);
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
    const boost::shared_ptr<v3d::render::realtime::frame::Pass> pass = frame->pass(uiPass);
    pass->keepColour();
    pass->depth(false);
    engine_.quads()->submit(canvas, pass.get());
}

/**
 **/
view::ConstructionPlane::Colours Renderer::gridColours() const {
    const v3d::ui::paint::Dressing& dress = std::as_const(screen_->components()).dressing();
    // a line is drawn opaque, so the theme's hairline is laid over the view's background here
    const auto over = [this](const glm::vec4& colour, float alpha) {
        return glm::vec4(glm::mix(glm::vec3(background_), glm::vec3(colour), std::clamp(alpha, 0.0F, 1.0F)), 1.0F);
    };
    return { .minor = over(dress.hairline, dress.hairline.a), .major = over(dress.hairline, dress.hairline.a * GRID_MAJOR),
        .axes = { over(view::OverlayLook::X_AXIS, GRID_AXIS), over(view::OverlayLook::Y_AXIS, GRID_AXIS), over(view::OverlayLook::Z_AXIS, GRID_AXIS) } };
}

/**
 **/
void Renderer::retire(v3d::render::realtime::frame::Frame* frame, const std::vector<boost::shared_ptr<view::ViewPort>>& views) {
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

    // the bar's text is a step smaller than the ui's, as secondary text is
    const float size = screen_->size() * STATUS_TEXT;
    const v3d::ui::paint::Type type(screen_->text(), size);
    // the frame and the rate change as they are read, so their figures are all one width
    const v3d::ui::paint::Type figures = type.tabular();
    const float middle = top + (dress.barHeight / 2.0F);
    const float baseline = middle + (size * BASELINE);
    // for each button: the key held with it on a chip, the mouse with the button lit, and what
    // it does, then a wider gap before the next
    float x = left + dress.padding;
    for (const control::chrome::Status::Button& button : status_.buttons) {
        if (!button.modifier.empty()) {
            const float chip = type.width(button.modifier) + (dress.padding * STATUS_CHIP_INSET * 2.0F);
            v3d::ui::paint::fillBox(canvas, glm::vec2(x, middle - (size * STATUS_CHIP)), glm::vec2(x + chip, middle + (size * STATUS_CHIP)), dress.radius,
                dress.widget);
            type.draw(canvas, button.modifier, glm::vec2(x + (dress.padding * STATUS_CHIP_INSET), baseline), dress.text);
            x += chip + (size * STATUS_AFTER_CHIP);
        }
        const v3d::ui::Image mouse = icon(button.icon);
        if (mouse.valid()) {
            const float side = size * STATUS_ICON;
            canvas->rect(glm::vec2(x, middle - (side / 2.0F)), glm::vec2(x + side, middle + (side / 2.0F)), mouse.uv0, mouse.uv1, dress.secondaryText,
                mouse.texture);
            x += side + (size * STATUS_AFTER_ICON);
        } else {
            type.draw(canvas, button.name, glm::vec2(x, baseline), dress.secondaryText);
            x += type.width(button.name) + (size / 2.0F);
        }
        type.draw(canvas, button.action, glm::vec2(x, baseline), dress.text);
        x += type.width(button.action) + dress.padding;
    }

    // the selection and the frame, then the frame rate in the colour of how it stands
    std::string right;
    for (const std::string* part : { &status_.selection, &status_.frame }) {
        if (!part->empty()) {
            right += (right.empty() ? "" : STATUS_GAP) + *part;
        }
    }
    // the rate keeps a slot wider than it needs, so the text before it does not move as it changes
    float end = width - dress.padding;
    if (!status_.rate.empty()) {
        figures.draw(canvas, status_.rate, glm::vec2(end - figures.width(status_.rate), baseline), healthColour(status_.health));
        end -= control::chrome::StatusBar::rateRoom(figures, status_.rate) + figures.width(STATUS_GAP);
    }
    figures.draw(canvas, right, glm::vec2(end - figures.width(right), baseline), dress.secondaryText);
}

/**
 **/
void Renderer::drawDividerHint(v3d::render::realtime::canvas::Canvas* canvas) {
    if (!pointer_ || !dividerHint_.due({ .lit = pointer_->lit().has_value(), .held = pointer_->holding() }, screen_->clock().now)) {
        return;
    }
    const v3d::ui::paint::Dressing& dress = std::as_const(screen_->components()).dressing();
    const float size = screen_->size();
    const v3d::ui::paint::Type type(screen_->text(), size);
    float wide = 0.0F;
    for (const std::string_view line : control::chrome::DividerHint::LINES) {
        wide = std::max(wide, type.width(line));
    }
    const glm::vec2 plate(wide + (dress.padding * 2.0F), (dress.lineHeight * static_cast<float>(control::chrome::DividerHint::LINES.size())) + dress.padding);
    // beside the cursor, and kept on the canvas
    const glm::vec2 room(static_cast<float>(canvas->width()), static_cast<float>(canvas->height()));
    const glm::vec2 offset = glm::vec2(HINT_OFFSET) * screen_->factor();
    const glm::vec2 min = glm::round(glm::clamp(pointer_->cursor() + offset, glm::vec2(0.0F), glm::max(room - plate, glm::vec2(0.0F))));
    const glm::vec2 max = min + plate;
    const glm::vec2 drop(0.0F, dress.shadowOffset);
    v3d::ui::paint::shadowBox(canvas, min + drop, max + drop, { .radius = dress.radius, .reach = dress.shadowSize, .colour = dress.shadow });
    v3d::ui::paint::plateBox(canvas, min, max, dress.radius, dress.borderWidth, { .inside = dress.panel, .outline = dress.border });
    float top = min.y + (dress.padding / 2.0F);
    for (std::size_t index = 0; index < control::chrome::DividerHint::LINES.size(); index++) {
        type.draw(canvas, control::chrome::DividerHint::LINES[index], glm::vec2(min.x + dress.padding, top + (dress.lineHeight / 2.0F) + (size * BASELINE)),
            index == 0 ? dress.text : dress.secondaryText);
        top += dress.lineHeight;
    }
}

/**
 **/
v3d::ui::Image Renderer::icon(const std::string& source) const {
    const std::optional<v3d::ui::Image::Corners> corners = icons_.corners(source);
    return corners && iconTexture_.valid() ? v3d::ui::Image(iconTexture_, *corners) : v3d::ui::Image();
}

/**
 **/
glm::vec4 Renderer::healthColour(control::chrome::Status::Health health) {
    switch (health) {
        case control::chrome::Status::Health::Good:
            return HEALTH_GOOD;
        case control::chrome::Status::Health::Low:
            return HEALTH_LOW;
        case control::chrome::Status::Health::Poor:
            return HEALTH_POOR;
    }
    return HEALTH_GOOD;
}

/**
 **/
void Renderer::drawNotice(v3d::render::realtime::canvas::Canvas* canvas) {
    if (!notices_ || !views_ || !pointer_ || views_->layout()->areas().empty()) {
        return;
    }
    const std::optional<control::chrome::Notices::Shown> shown = notices_->shown(screen_->clock().now);
    const glm::vec4& content = views_->layout()->areas()[pointer_->activeArea()].content;
    if (!shown || content.z <= 0.0F || content.w <= 0.0F) {
        return;
    }
    const v3d::ui::paint::Dressing& dress = std::as_const(screen_->components()).dressing();
    const float size = screen_->size();
    const v3d::ui::paint::Type type(screen_->text(), size);

    // a mark for done or refused, the message, and how to undo a change that can be undone
    const v3d::ui::Image mark = icon(shown->done ? std::string(NOTICE_DONE) : std::string(NOTICE_REFUSED));
    const float side = mark.valid() ? size : 0.0F;
    const float gap = size * NOTICE_GAP;
    const std::string keys = shown->undoable ? screen_->shortcut(v3d::event::Event(std::string(UNDO_COMMAND))) : std::string();
    const std::string hint = keys.empty() ? std::string() : keys + " to undo";
    const float hintWidth = hint.empty() ? 0.0F : gap + type.width(hint);
    const float markWidth = side > 0.0F ? side + gap : 0.0F;
    const glm::vec2 plate(markWidth + type.width(shown->text) + hintWidth + (dress.padding * 2.0F), dress.barHeight);
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
    const float middle = min.y + (plate.y / 2.0F);
    const float baseline = middle + (size * BASELINE);
    float pen = min.x + dress.padding;
    if (mark.valid()) {
        canvas->rect(glm::vec2(pen, middle - (side / 2.0F)), glm::vec2(pen + side, middle + (side / 2.0F)), mark.uv0, mark.uv1,
            shown->done ? HEALTH_GOOD : HEALTH_POOR, mark.texture);
        pen += markWidth;
    }
    type.draw(canvas, shown->text, glm::vec2(pen, baseline), dress.text);
    if (!hint.empty()) {
        type.draw(canvas, hint, glm::vec2(pen + type.width(shown->text) + gap, baseline), dress.secondaryText);
    }
    canvas->unfade();
}

/**
 **/
v3d::render::realtime::resource::TextureHandle Renderer::picture(const boost::shared_ptr<v3d::image::Image>& image) {
    // the ui stops drawing the old texture this frame, and its image is reclaimed once the
    // frame in flight has finished with it
    if (picture_.valid()) {
        engine_.textures()->release(picture_);
        picture_ = v3d::render::realtime::resource::TextureHandle();
    }
    if (image) {
        picture_ = engine_.textures()->texture(image);
    }
    return picture_;
}

/**
 **/
void Renderer::shutdown() {
    // each other window's surface has to go before the window does, which the engine destroys
    // after the app's release()
    while (!others_.empty()) {
        close(others_.back()->window);
    }
    // the surfaces' buffers go back to the ring, which destroys them once the device is idle
    shaded_.reset();
    engine_.shutdown();
}

};  // namespace v3d::editor
