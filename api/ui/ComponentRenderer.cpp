/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "ComponentRenderer.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include "Container.h"
#include "Engine.h"
#include "Painter.h"
#include "Style.h"
#include "component/Bar.h"
#include "component/Box.h"
#include "component/Button.h"
#include "component/CheckBox.h"
#include "component/Icon.h"
#include "component/Label.h"
#include "component/Panel.h"
#include "component/RadioButton.h"
#include "component/Scrollbar.h"
#include "component/SelectList.h"
#include "component/TabBar.h"
#include "component/TabPage.h"
#include "component/Toolbar.h"
#include "component/Type.h"
#include "component/menu/Menu.h"
#include "component/menu/MenuBar.h"
#include "style/Button.h"
#include "style/Theme.h"
#include "style/property/Color.h"
#include "style/property/Image.h"
#include "style/property/Number.h"

namespace v3d::ui {

namespace {

/**
 * The width of the columns either side of a dropped panel's labels, as a fraction of
 * a row's height: the mark a checked item draws on the left and the arrow a submenu
 * item draws on the right.
 **/
const float markColumn = 0.9f;

/**
 * The rule drawn along the far edge of a strip, which is what makes it read as
 * something over the scene rather than as part of it.
 **/
const float ruleWidth = 1.0f;

/**
 * How many segments a radio button's disc is drawn with. Small enough to cost little at
 * the size a mark is drawn, large enough that the rim does not read as a polygon.
 **/
const unsigned int markSides = 16;

/**
 * How much of the box, or of the disc, a mark fills. The rest is the gap that makes it
 * read as a mark inside something rather than as a filled box.
 **/
const float markFill = 0.5f;

/**
 * How far in from the edge of a skinned button the corner images reach, when the
 * style names no corner of its own. A texture carries no size a handle can be asked
 * for, so this is a number rather than something measured.
 **/
const float defaultCorner = 8.0f;

/**
 * Leave a component holding the bounds it was drawn in, which is what the cursor is
 * tested against per ADR-0019.
 *
 * Through a reference to the base, because a menu's own size() is its item count and
 * hides the one that means how big it is.
 **/
void place(Component& component, const glm::vec2& position, const glm::vec2& size) {
    component.position(position);
    component.size(size);
}

/**
 * @return the texture a style's image property was resolved to, unset when the style
 *      names no such image or nothing has resolved it
 **/
v3d::render::realtime::TextureHandle image(const boost::shared_ptr<Style>& target, const std::string& name) {
    boost::shared_ptr<style::property::Image> property =
        boost::dynamic_pointer_cast<style::property::Image>(target->property(name, "image"));
    return property ? property->texture() : v3d::render::realtime::TextureHandle();
}

};  // namespace

/**
 **/
ComponentRenderer::Dressing::Dressing() noexcept :
lineHeight(34.0f),
padding(24.0f),
barHeight(28.0f),
iconSize(22.0f),
panelPadding(4.0f),
scrollbarWidth(12.0f),
markSize(16.0f),
borderWidth(1.0f),
radius(0.0f),
panel(0.05f, 0.06f, 0.09f, 0.92f),
border(0.35f, 0.38f, 0.45f, 1.0f),
track(0.12f, 0.13f, 0.17f, 1.0f),
fill(0.30f, 0.62f, 0.36f, 1.0f),
thumb(0.35f, 0.38f, 0.45f, 1.0f),
mark(0.42f, 0.66f, 0.95f, 1.0f),
text(0.78f, 0.80f, 0.84f, 1.0f),
activeText(1.0f, 1.0f, 1.0f, 1.0f),
highlight(0.16f, 0.34f, 0.58f, 1.0f),
hover(0.16f, 0.18f, 0.24f, 1.0f) {
}

/**
 **/
ComponentRenderer::ComponentRenderer(const Measure& measure, const Write& write) :
    measure_(measure),
    write_(write) {
}

// out of line, so that the header need not complete the types the members hold
ComponentRenderer::~ComponentRenderer() {
}

/**
 **/
ComponentRenderer::Dressing& ComponentRenderer::dressing() noexcept {
    return dressing_;
}

/**
 **/
void ComponentRenderer::theme(const boost::shared_ptr<style::Theme>& theme) {
    theme_ = theme;
    if (!theme_) {
        return;
    }

    const boost::shared_ptr<v3d::ui::Style> chrome = lookup("ui", std::string_view());
    if (!chrome) {
        return;
    }

    readColour(chrome, "panel", &dressing_.panel);
    readColour(chrome, "border", &dressing_.border);
    readColour(chrome, "track", &dressing_.track);
    readColour(chrome, "fill", &dressing_.fill);
    readColour(chrome, "thumb", &dressing_.thumb);
    readColour(chrome, "mark", &dressing_.mark);
    readColour(chrome, "text", &dressing_.text);
    readColour(chrome, "active-text", &dressing_.activeText);
    readColour(chrome, "highlight", &dressing_.highlight);
    readColour(chrome, "hover", &dressing_.hover);

    readMetric(chrome, "line-height", &dressing_.lineHeight);
    readMetric(chrome, "padding", &dressing_.padding);
    readMetric(chrome, "bar-height", &dressing_.barHeight);
    readMetric(chrome, "icon-size", &dressing_.iconSize);
    readMetric(chrome, "panel-padding", &dressing_.panelPadding);
    readMetric(chrome, "scrollbar-width", &dressing_.scrollbarWidth);
    readMetric(chrome, "mark-size", &dressing_.markSize);
    readMetric(chrome, "border-width", &dressing_.borderWidth);
    readMetric(chrome, "radius", &dressing_.radius);
}

/**
 **/
boost::shared_ptr<style::Theme> ComponentRenderer::theme() const noexcept {
    return theme_;
}

/**
 **/
boost::shared_ptr<v3d::ui::Style> ComponentRenderer::lookup(const std::string& className, const std::string_view& name) const {
    if (!theme_) {
        return nullptr;
    }
    const std::vector<boost::shared_ptr<v3d::ui::Style>> styles =
        theme_->getStyleSet(std::string(name), className);
    return styles.empty() ? nullptr : styles.front();
}

/**
 **/
void ComponentRenderer::draw(v3d::render::realtime::Canvas* canvas, const Engine& ui) const {
    for (const boost::shared_ptr<Container>& container : ui.containers()) {
        if (container && container->visible()) {
            draw(canvas, *container);
        }
    }
}

/**
 **/
void ComponentRenderer::draw(v3d::render::realtime::Canvas* canvas, const Container& container) const {
    if (canvas == nullptr) {
        return;
    }
    // what the strips drawn so far have taken off the top and the left edges, which is
    // where the next one starts
    glm::vec2 taken(0.0f, 0.0f);
    // the box every root component is laid out against
    const v3d::type::Bound2D area(glm::vec2(0.0f, 0.0f),
        glm::vec2(static_cast<float>(canvas->width()), static_cast<float>(canvas->height())));
    std::vector<boost::shared_ptr<component::MenuBar>> bars;
    for (const boost::shared_ptr<Component>& component : container.ordered()) {
        if (!component || !component->visible()) {
            continue;
        }
        if (component->type() == component::Type::Menu) {
            draw(canvas, boost::dynamic_pointer_cast<component::Menu>(component));
        } else if (component->type() == component::Type::MenuBar) {
            // held back to the end: an open menu drops a panel over whatever the strips
            // below it occupy, so it has to be drawn after them
            bars.push_back(boost::dynamic_pointer_cast<component::MenuBar>(component));
            taken.y += dressing_.barHeight + ruleWidth;
        } else if (component->type() == component::Type::Toolbar) {
            const boost::shared_ptr<component::Toolbar> bar =
                boost::dynamic_pointer_cast<component::Toolbar>(component);
            if (!bar) {
                continue;
            }
            if (bar->edge() == component::Toolbar::Edge::Top) {
                draw(canvas, bar, glm::vec2(0.0f, taken.y));
                taken.y += dressing_.barHeight + ruleWidth;
            } else {
                draw(canvas, bar, glm::vec2(taken.x, taken.y));
                taken.x += bar->bound().size().x + ruleWidth;
            }
        } else {
            // everything else is a box: it is laid out against the canvas, and whatever it
            // holds is laid out against it
            walk(canvas, component, component->layout().resolve(area, natural(*component), component->position()));
        }
    }
    for (const boost::shared_ptr<component::MenuBar>& bar : bars) {
        draw(canvas, bar);
    }
}

/**
 **/
void ComponentRenderer::draw(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::Label>& label) const {
    if (canvas == nullptr || !label) {
        return;
    }
    const std::string text(label->text());
    glm::vec2 size = label->size();
    if (size.x <= 0.0f || size.y <= 0.0f) {
        size = glm::vec2(measure_(text), dressing_.lineHeight);
    }
    place(*label, label->position(), size);

    const glm::vec2 pen(label->position().x, label->position().y + dressing_.lineHeight * 0.75f);
    write_(text, pen, dressing_.text);
}

/**
 **/
void ComponentRenderer::draw(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::Icon>& icon) const {
    if (canvas == nullptr || !icon || !icon->texture().valid()) {
        return;
    }
    // an icon given no size is a square the height of a strip, which is the one size the
    // ui has that is not derived from a string
    glm::vec2 size = icon->size();
    if (size.x <= 0.0f || size.y <= 0.0f) {
        size = glm::vec2(dressing_.barHeight, dressing_.barHeight);
    }
    place(*icon, icon->position(), size);

    canvas->rect(icon->position(), icon->position() + size,
        glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), icon->texture());
}

/**
 **/
void ComponentRenderer::draw(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::Button>& button) const {
    if (canvas == nullptr || !button) {
        return;
    }
    const std::string label(button->label());

    glm::vec2 size = button->size();
    if (size.x <= 0.0f || size.y <= 0.0f) {
        size = glm::vec2(extent(*button) + dressing_.padding, dressing_.barHeight);
    }
    const glm::vec2 min = button->position();
    place(*button, min, size);

    // a checked toggle keeps its highlight whether or not the cursor is on it, which is
    // what says which mask and which tool are in force
    const bool lit = button->checked() || button->state() == component::Button::STATE_HOVER;
    if (!skin(canvas, *button, min, min + size) && lit) {
        canvas->rect(min, min + size, button->checked() ? dressing_.highlight : dressing_.hover);
    }

    // an icon is what the button says instead of its label, not as well as it. The label
    // stays on the component for whatever measures it before an image has been resolved
    if (button->texture().valid()) {
        const float side = std::min(dressing_.iconSize, std::min(size.x, size.y));
        const glm::vec2 corner = min + (size - glm::vec2(side, side)) * 0.5f;
        canvas->rect(corner, corner + glm::vec2(side, side),
            glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), button->texture());
        return;
    }

    const glm::vec2 baseline(min.x + (size.x - measure_(label)) * 0.5f, min.y + size.y * 0.7f);
    write_(label, baseline, lit ? dressing_.activeText : dressing_.text);
}

/**
 **/
void ComponentRenderer::draw(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::Panel>& panel) const {
    if (canvas == nullptr || !panel) {
        return;
    }
    glm::vec4 inside = dressing_.panel;
    glm::vec4 outline = dressing_.border;
    float width = dressing_.borderWidth;
    float radius = dressing_.radius;
    const boost::shared_ptr<v3d::ui::Style> dress = lookup("panel", panel->style());
    if (dress) {
        readColour(dress, "background", &inside);
        readColour(dress, "border", &outline);
        readMetric(dress, "border-width", &width);
        readMetric(dress, "radius", &radius);
    }
    const glm::vec2 min = panel->position();
    plateBox(canvas, min, min + panel->size(), radius, width, inside, outline);
}

/**
 **/
void ComponentRenderer::draw(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::Bar>& bar) const {
    if (canvas == nullptr || !bar) {
        return;
    }
    glm::vec4 empty = dressing_.track;
    glm::vec4 filled = dressing_.fill;
    glm::vec4 outline = dressing_.border;
    float width = dressing_.borderWidth;
    float radius = dressing_.radius;
    const boost::shared_ptr<v3d::ui::Style> dress = lookup("bar", bar->style());
    if (dress) {
        readColour(dress, "track", &empty);
        readColour(dress, "fill", &filled);
        readColour(dress, "border", &outline);
        readMetric(dress, "border-width", &width);
        readMetric(dress, "radius", &radius);
    }

    const glm::vec2 min = bar->position();
    const glm::vec2 max = min + bar->size();
    plateBox(canvas, min, max, radius, width, empty, outline);
    if (bar->fraction() <= 0.0f) {
        return;
    }

    // the fill sits inside the border rather than under it, so a bar at full still reads
    // as something in a track
    const glm::vec2 inset(width, width);
    glm::vec2 low = min + inset;
    glm::vec2 high = max - inset;
    if (bar->direction() == component::Bar::Direction::Horizontal) {
        high.x = low.x + (high.x - low.x) * bar->fraction();
    } else {
        // a vertical bar fills from the bottom, which is the way one is read
        low.y = high.y - (high.y - low.y) * bar->fraction();
    }
    fillBox(canvas, low, high, std::max(0.0f, radius - width), filled);
}

/**
 **/
void ComponentRenderer::draw(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::Scrollbar>& bar) const {
    if (canvas == nullptr || !bar) {
        return;
    }
    glm::vec4 empty = dressing_.track;
    glm::vec4 grip = dressing_.thumb;
    glm::vec4 outline = dressing_.border;
    float width = dressing_.borderWidth;
    float radius = dressing_.radius;
    // a scrollbar is not a progress bar: it dresses from its own style class, so a theme
    // that paints a health bar green does not paint a scrollbar green as well
    const boost::shared_ptr<v3d::ui::Style> dress = lookup("scrollbar", bar->style());
    if (dress) {
        readColour(dress, "track", &empty);
        readColour(dress, "thumb", &grip);
        readColour(dress, "border", &outline);
        readMetric(dress, "border-width", &width);
        readMetric(dress, "radius", &radius);
    }

    const glm::vec2 min = bar->position();
    const glm::vec2 max = min + bar->size();
    plateBox(canvas, min, max, radius, width, empty, outline);
    if (!bar->scrollable()) {
        // a page showing all of its content has a thumb the length of the track, which
        // would read as a bar scrolled nowhere rather than as one with nowhere to go
        return;
    }

    // the thumb sits inside the border, the way a bar's fill does
    const glm::vec2 inset(width, width);
    glm::vec2 low = min + inset;
    glm::vec2 high = max - inset;
    if (bar->direction() == component::Scrollbar::Direction::Vertical) {
        low.y = min.y + bar->thumbStart();
        high.y = low.y + bar->thumb();
    } else {
        low.x = min.x + bar->thumbStart();
        high.x = low.x + bar->thumb();
    }
    fillBox(canvas, low, high, std::max(0.0f, radius - width), grip);
}

/**
 **/
void ComponentRenderer::draw(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::CheckBox>& box) const {
    if (canvas == nullptr || !box) {
        return;
    }
    const bool round = box->type() == component::Type::RadioButton;
    const std::string text(box->label());

    glm::vec2 size = box->size();
    if (size.x <= 0.0f || size.y <= 0.0f) {
        size = natural(*box);
    }
    const glm::vec2 min = box->position();
    place(*box, min, size);

    glm::vec4 inside = dressing_.track;
    glm::vec4 marked = dressing_.mark;
    glm::vec4 outline = dressing_.border;
    glm::vec4 ink = dressing_.text;
    float width = dressing_.borderWidth;
    float side = std::min(dressing_.markSize, size.y);
    const boost::shared_ptr<v3d::ui::Style> dress = lookup(round ? "radio" : "checkbox", box->style());
    if (dress) {
        readColour(dress, "background", &inside);
        readColour(dress, "mark", &marked);
        readColour(dress, "border", &outline);
        readColour(dress, "text", &ink);
        readMetric(dress, "border-width", &width);
        readMetric(dress, "mark-size", &side);
    }

    // the mark is centred in the row rather than sitting on its top edge, because the
    // label beside it is centred too
    const glm::vec2 corner(min.x, min.y + (size.y - side) * 0.5f);
    if (round) {
        const glm::vec2 centre = corner + glm::vec2(side, side) * 0.5f;
        canvas->circle(centre, side * 0.5f, markSides, outline);
        canvas->circle(centre, side * 0.5f - width, markSides, inside);
        if (box->checked()) {
            canvas->circle(centre, side * markFill * 0.5f, markSides, marked);
        }
    } else {
        plateBox(canvas, corner, corner + glm::vec2(side, side), dressing_.radius, width, inside, outline);
        if (box->checked()) {
            const float inset = side * (1.0f - markFill) * 0.5f;
            fillBox(canvas, corner + glm::vec2(inset, inset), corner + glm::vec2(side - inset, side - inset),
                std::max(0.0f, dressing_.radius - width), marked);
        }
    }

    if (text.empty()) {
        return;
    }
    write_(text, glm::vec2(min.x + side + dressing_.padding * 0.5f, min.y + size.y * 0.7f), ink);
}

/**
 **/
void ComponentRenderer::draw(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::SelectList>& list) const {
    if (canvas == nullptr || !list) {
        return;
    }
    glm::vec2 size = list->size();
    if (size.x <= 0.0f || size.y <= 0.0f) {
        size = natural(*list);
    }
    const glm::vec2 min = list->position();
    place(*list, min, size);

    glm::vec4 inside = dressing_.panel;
    glm::vec4 outline = dressing_.border;
    glm::vec4 chosen = dressing_.highlight;
    glm::vec4 ink = dressing_.text;
    glm::vec4 chosenInk = dressing_.activeText;
    float width = dressing_.borderWidth;
    float radius = dressing_.radius;
    float row = dressing_.lineHeight;
    const boost::shared_ptr<v3d::ui::Style> dress = lookup("list", list->style());
    if (dress) {
        readColour(dress, "background", &inside);
        readColour(dress, "border", &outline);
        readColour(dress, "highlight", &chosen);
        readColour(dress, "text", &ink);
        readColour(dress, "active-text", &chosenInk);
        readMetric(dress, "border-width", &width);
        readMetric(dress, "radius", &radius);
        readMetric(dress, "line-height", &row);
    }

    plateBox(canvas, min, min + size, radius, width, inside, outline);

    // how tall a row is is the style's, and the list is what answers a point with it - so
    // it is written on the way past, the way a box is
    list->rowHeight(row);
    if (list->items().empty() || row <= 0.0f) {
        return;
    }

    const glm::vec2 low(min.x + width, min.y + width);
    const glm::vec2 high(min.x + size.x - width, min.y + size.y - width);
    canvas->clip(low, high);

    const float scrolled = list->offset();
    // only the rows the box shows are drawn - a list of a thousand costs the rows on screen
    const std::size_t first = static_cast<std::size_t>(std::max(scrolled / row, 0.0f));
    const std::size_t last = std::min(list->items().size(),
        first + static_cast<std::size_t>(size.y / row) + 2U);

    for (std::size_t index = first; index < last; index++) {
        const float top = min.y + static_cast<float>(index) * row - scrolled;
        const bool picked = static_cast<int>(index) == list->selected();
        if (picked) {
            fillBox(canvas, glm::vec2(low.x, top), glm::vec2(high.x, top + row), 0.0f, chosen);
        }
        write_(list->items()[index], glm::vec2(low.x + dressing_.padding * 0.5f, top + row * 0.7f),
            picked ? chosenInk : ink);
    }

    canvas->unclip();
}

/**
 **/
void ComponentRenderer::draw(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::TabBar>& bar) const {
    if (canvas == nullptr || !bar) {
        return;
    }
    glm::vec2 size = bar->size();
    if (size.x <= 0.0f || size.y <= 0.0f) {
        size = natural(*bar);
    }
    const glm::vec2 min = bar->position();
    place(*bar, min, size);

    glm::vec4 inside = dressing_.panel;
    glm::vec4 tab = dressing_.track;
    glm::vec4 chosen = dressing_.highlight;
    glm::vec4 ink = dressing_.text;
    glm::vec4 chosenInk = dressing_.activeText;
    glm::vec4 outline = dressing_.border;
    float height = dressing_.barHeight;
    float radius = dressing_.radius;
    const boost::shared_ptr<v3d::ui::Style> dress = lookup("tabs", bar->style());
    if (dress) {
        readColour(dress, "background", &inside);
        readColour(dress, "tab", &tab);
        readColour(dress, "highlight", &chosen);
        readColour(dress, "text", &ink);
        readColour(dress, "active-text", &chosenInk);
        readColour(dress, "border", &outline);
        readMetric(dress, "bar-height", &height);
        readMetric(dress, "radius", &radius);
    }

    const std::vector<boost::shared_ptr<component::TabPage>> pages = bar->pages();
    std::vector<v3d::type::Bound2D> boxes;
    boxes.reserve(pages.size());

    float pen = min.x;
    for (std::size_t index = 0; index < pages.size(); index++) {
        const std::string label(pages[index]->label());
        const float width = measure_(label) + dressing_.padding;
        const glm::vec2 corner(pen, min.y);
        const glm::vec2 extent(width, height);
        boxes.push_back(v3d::type::Bound2D(corner, extent));

        const bool picked = static_cast<int>(index) == bar->selected();
        fillBox(canvas, corner, corner + extent, radius, picked ? chosen : tab);
        write_(label, glm::vec2(corner.x + dressing_.padding * 0.5f, corner.y + height * 0.7f),
            picked ? chosenInk : ink);
        pen += width + dressing_.borderWidth;
    }
    // where each tab ended up, for the cursor to be tested against - the same rule as a
    // component's own box, per ADR-0019
    bar->tabs(boxes);

    // the rule under the strip, which is what joins the chosen tab to the page below it
    canvas->rect(glm::vec2(min.x, min.y + height), glm::vec2(min.x + size.x, min.y + height + ruleWidth), outline);

    const boost::shared_ptr<component::TabPage> page = bar->page();
    if (!page) {
        return;
    }
    const glm::vec2 corner(min.x, min.y + height + ruleWidth);
    const glm::vec2 extent(size.x, std::max(size.y - height - ruleWidth, 0.0f));
    fillBox(canvas, corner, corner + extent, 0.0f, inside);
    // a page is drawn where the strip left room, and what it holds is laid out inside that
    walk(canvas, page, v3d::type::Bound2D(corner, extent));
}

/**
 **/
glm::vec2 ComponentRenderer::natural(const Component& component) const {
    switch (component.type()) {
        case component::Type::Label: {
            const auto* label = dynamic_cast<const component::Label*>(&component);
            return label == nullptr ? glm::vec2(0.0f, 0.0f)
                : glm::vec2(measure_(std::string(label->text())), dressing_.lineHeight);
        }
        case component::Type::Icon:
            // an icon given no size is a square the height of a strip, which is the one
            // size the ui has that is not derived from a string
            return glm::vec2(dressing_.barHeight, dressing_.barHeight);
        case component::Type::Button: {
            const auto* button = dynamic_cast<const component::Button*>(&component);
            return button == nullptr ? glm::vec2(0.0f, 0.0f)
                : glm::vec2(extent(*button) + dressing_.padding, dressing_.barHeight);
        }
        case component::Type::CheckBox:
        case component::Type::RadioButton: {
            // the mark, the gap after it and the label, on a row as tall as the taller of
            // the mark and a line of text
            const auto* box = dynamic_cast<const component::CheckBox*>(&component);
            if (box == nullptr) {
                return glm::vec2(0.0f, 0.0f);
            }
            const float text = box->label().empty() ? 0.0f
                : dressing_.padding * 0.5f + measure_(std::string(box->label()));
            return glm::vec2(dressing_.markSize + text, std::max(dressing_.markSize, dressing_.lineHeight));
        }
        case component::Type::SelectList: {
            // a list decides how wide its widest row is and nothing about how tall it is:
            // how many rows it shows is what it was given room for
            const auto* list = dynamic_cast<const component::SelectList*>(&component);
            if (list == nullptr) {
                return glm::vec2(0.0f, 0.0f);
            }
            float widest = 0.0f;
            for (const std::string& item : list->items()) {
                widest = std::max(widest, measure_(item));
            }
            return glm::vec2(widest + dressing_.padding, component.size().y);
        }
        case component::Type::Scrollbar: {
            // a scrollbar decides how thick it is and nothing about how long: its length
            // is the box it runs down, which is its parent's rather than its own
            const auto* bar = dynamic_cast<const component::Scrollbar*>(&component);
            if (bar == nullptr) {
                return glm::vec2(0.0f, 0.0f);
            }
            return bar->direction() == component::Scrollbar::Direction::Vertical
                ? glm::vec2(dressing_.scrollbarWidth, component.size().y)
                : glm::vec2(component.size().x, dressing_.scrollbarWidth);
        }
        default:
            // a panel, a bar and a box decide nothing for themselves, so an Auto extent on
            // one is whatever it was last given
            return component.size();
    }
}

/**
 **/
void ComponentRenderer::arrange(const component::Box& box, const v3d::type::Bound2D& bounds,
    std::vector<v3d::type::Bound2D>* boxes) const {
    const bool vertical = box.type() == component::Type::VerticalBox;
    const glm::vec2 extent = bounds.size();
    float pen = vertical ? bounds.position().y : bounds.position().x;

    for (const boost::shared_ptr<Component>& child : box.children()) {
        if (!child || !child->visible()) {
            // a hidden row leaves no gap behind it, which is what makes a list of however
            // many rows there are read as one
            boxes->push_back(v3d::type::Bound2D(bounds.position(), glm::vec2(0.0f, 0.0f)));
            continue;
        }
        const glm::vec2 own = natural(*child);
        const Layout& layout = child->layout();
        glm::vec2 size(layout.width.resolve(extent.x, own.x), layout.height.resolve(extent.y, own.y));
        glm::vec2 corner;
        if (vertical) {
            if (box.stretch()) {
                size.x = extent.x;
            }
            corner = glm::vec2(bounds.position().x + layout.x.resolve(extent.x, 0.0f), pen);
            pen += size.y + box.spacing();
        } else {
            if (box.stretch()) {
                size.y = extent.y;
            }
            corner = glm::vec2(pen, bounds.position().y + layout.y.resolve(extent.y, 0.0f));
            pen += size.x + box.spacing();
        }
        boxes->push_back(v3d::type::Bound2D(corner, size));
    }
}

/**
 **/
void ComponentRenderer::walk(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<Component>& component,
    const v3d::type::Bound2D& bounds) const {
    if (canvas == nullptr || !component || !component->visible()) {
        return;
    }
    place(*component, bounds.position(), bounds.size());

    switch (component->type()) {
        case component::Type::Panel:
            draw(canvas, boost::dynamic_pointer_cast<component::Panel>(component));
            break;
        case component::Type::Bar:
            draw(canvas, boost::dynamic_pointer_cast<component::Bar>(component));
            break;
        case component::Type::Scrollbar:
            draw(canvas, boost::dynamic_pointer_cast<component::Scrollbar>(component));
            break;
        case component::Type::CheckBox:
        case component::Type::RadioButton:
            // a radio button is a check box with a round mark, so one call draws both
            draw(canvas, boost::dynamic_pointer_cast<component::CheckBox>(component));
            break;
        case component::Type::SelectList:
            draw(canvas, boost::dynamic_pointer_cast<component::SelectList>(component));
            break;
        case component::Type::TabBar:
            // a bar walks the one page it shows, so the pages behind it are not laid out
            // and the generic walk below must not reach them
            draw(canvas, boost::dynamic_pointer_cast<component::TabBar>(component));
            return;
        case component::Type::Button:
            draw(canvas, boost::dynamic_pointer_cast<component::Button>(component));
            break;
        case component::Type::Label:
            draw(canvas, boost::dynamic_pointer_cast<component::Label>(component));
            break;
        case component::Type::Icon:
            draw(canvas, boost::dynamic_pointer_cast<component::Icon>(component));
            break;
        default:
            // a box draws nothing of its own - it is whatever it holds
            break;
    }

    const std::vector<boost::shared_ptr<Component>>& children = component->children();
    if (children.empty()) {
        return;
    }

    // a component that holds more than it can show cuts what it holds off at its own box,
    // per ADR-0037. It is what the component asked for rather than the default, because a
    // menu drops a panel out of the strip it came from
    const bool cut = component->clip();
    if (cut) {
        canvas->clip(component->position(), component->position() + component->size());
    }

    const auto* box = dynamic_cast<const component::Box*>(component.get());
    if (box != nullptr) {
        // a flow box places its children in the order it holds them, because that order is
        // what it is for. A z index inside one changes nothing
        std::vector<v3d::type::Bound2D> boxes;
        boxes.reserve(children.size());
        arrange(*box, component->bound(), &boxes);
        for (std::size_t index = 0; index < children.size(); index++) {
            walk(canvas, children[index], boxes[index]);
        }
    } else {
        for (const boost::shared_ptr<Component>& child : v3d::ui::ordered(children)) {
            walk(canvas, child, child->layout().resolve(component->bound(), natural(*child), child->position()));
        }
    }

    if (cut) {
        canvas->unclip();
    }
}

/**
 **/
float ComponentRenderer::extent(const component::Button& button) const {
    // what the button asks a strip for, which is the icon it names rather than the
    // texture it holds - a strip is laid out before anything has been resolved
    if (!button.icon().empty()) {
        return dressing_.iconSize;
    }
    return measure_(std::string(button.label()));
}

/**
 **/
bool ComponentRenderer::skin(v3d::render::realtime::Canvas* canvas, const component::Button& button,
    const glm::vec2& min, const glm::vec2& max) const {
    if (!theme_) {
        return false;
    }

    // a button's styles are told apart by state as well as by name, so the set is walked
    // rather than asked for one
    boost::shared_ptr<v3d::ui::Style> target;
    for (const boost::shared_ptr<v3d::ui::Style>& candidate : theme_->getStyleSet(std::string(button.style()), "button")) {
        const boost::shared_ptr<style::Button> styled = boost::dynamic_pointer_cast<style::Button>(candidate);
        if (styled && styled->state() == button.state()) {
            target = styled;
            break;
        }
    }
    if (!target) {
        return false;
    }

    float corner = defaultCorner;
    readMetric(target, "corner", &corner);
    corner = std::min(corner, std::min((max.x - min.x) * 0.5f, (max.y - min.y) * 0.5f));

    const glm::vec2 uv0(0.0f, 0.0f);
    const glm::vec2 uv1(1.0f, 1.0f);
    const glm::vec4 white(1.0f, 1.0f, 1.0f, 1.0f);
    unsigned int drawn = 0;

    // every one of the nine is optional: a style naming only a centre is a flat skin, and
    // one naming none at all is not a skin, which is what leaves the button drawn flat
    const struct {
        const char* name;
        glm::vec2 min;
        glm::vec2 max;
    } parts[] = {
        { "top-left", min, min + glm::vec2(corner, corner) },
        { "top-right", glm::vec2(max.x - corner, min.y), glm::vec2(max.x, min.y + corner) },
        { "bottom-left", glm::vec2(min.x, max.y - corner), glm::vec2(min.x + corner, max.y) },
        { "bottom-right", max - glm::vec2(corner, corner), max },
        { "top", glm::vec2(min.x + corner, min.y), glm::vec2(max.x - corner, min.y + corner) },
        { "bottom", glm::vec2(min.x + corner, max.y - corner), glm::vec2(max.x - corner, max.y) },
        { "left", glm::vec2(min.x, min.y + corner), glm::vec2(min.x + corner, max.y - corner) },
        { "right", glm::vec2(max.x - corner, min.y + corner), glm::vec2(max.x, max.y - corner) },
        { "center", min + glm::vec2(corner, corner), max - glm::vec2(corner, corner) }
    };

    for (const auto& part : parts) {
        const v3d::render::realtime::TextureHandle texture = image(target, part.name);
        if (!texture.valid()) {
            continue;
        }
        canvas->rect(part.min, part.max, uv0, uv1, white, texture);
        drawn++;
    }

    return drawn > 0;
}

/**
 **/
glm::vec2 ComponentRenderer::insets(const Engine& ui) const {
    glm::vec2 total(0.0f, 0.0f);
    for (const boost::shared_ptr<Container>& container : ui.containers()) {
        if (container && container->visible()) {
            total += insets(*container);
        }
    }
    return total;
}

/**
 **/
glm::vec2 ComponentRenderer::insets(const Container& container) const {
    // the same walk draw() makes, and it has to stay the same: an app shrinks what it
    // draws by this and the ui is then drawn over the strip it was told about
    glm::vec2 taken(0.0f, 0.0f);
    for (const boost::shared_ptr<Component>& component : container.components()) {
        if (!component || !component->visible()) {
            continue;
        }
        if (component->type() == component::Type::MenuBar) {
            taken.y += dressing_.barHeight + ruleWidth;
        } else if (component->type() == component::Type::Toolbar) {
            const boost::shared_ptr<component::Toolbar> bar =
                boost::dynamic_pointer_cast<component::Toolbar>(component);
            if (!bar) {
                continue;
            }
            if (bar->edge() == component::Toolbar::Edge::Top) {
                taken.y += dressing_.barHeight + ruleWidth;
            } else {
                taken.x += widest(*bar) + dressing_.padding + ruleWidth;
            }
        }
    }
    return taken;
}

/**
 **/
void ComponentRenderer::draw(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::Menu>& menu) const {
    if (canvas == nullptr || !menu) {
        return;
    }

    boost::shared_ptr<component::Menu> level = menu->level();
    if (!level) {
        level = menu;
    }
    const std::size_t count = level->size();
    if (count == 0) {
        return;
    }

    std::vector<std::string> labels;
    labels.reserve(count);
    float widest = 0.0f;
    for (std::size_t index = 0; index < count; index++) {
        const boost::shared_ptr<component::MenuItem>& item = (*level)[index];
        const std::string label = item ? item->text() : std::string();
        widest = std::max(widest, measure_(label));
        labels.push_back(label);
    }

    const float width = widest + dressing_.padding * 2.0f;
    const float height = dressing_.lineHeight * static_cast<float>(count) + dressing_.padding * 2.0f;
    const glm::vec2 origin(
        (static_cast<float>(canvas->width()) - width) * 0.5f,
        (static_cast<float>(canvas->height()) - height) * 0.5f);

    // a one pixel border, as a filled rectangle with the panel drawn over it
    canvas->rect(origin - glm::vec2(1.0f, 1.0f), origin + glm::vec2(width + 1.0f, height + 1.0f), dressing_.border);
    canvas->rect(origin, origin + glm::vec2(width, height), dressing_.panel);

    const boost::shared_ptr<component::MenuItem> active = level->active();

    for (std::size_t index = 0; index < count; index++) {
        const float top = origin.y + dressing_.padding + dressing_.lineHeight * static_cast<float>(index);
        const bool selected = active && (*level)[index] == active;

        if (selected) {
            canvas->rect(
                glm::vec2(origin.x + dressing_.padding * 0.5f, top),
                glm::vec2(origin.x + width - dressing_.padding * 0.5f, top + dressing_.lineHeight),
                dressing_.highlight);
        }

        // the pen sits on the baseline, which is most of the way down the line box - the
        // remainder is where descenders go
        const glm::vec2 pen(origin.x + dressing_.padding, top + dressing_.lineHeight * 0.75f);
        write_(labels[index], pen, selected ? dressing_.activeText : dressing_.text);
    }
}

/**
 **/
void ComponentRenderer::draw(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::MenuBar>& bar) const {
    if (canvas == nullptr || !bar) {
        return;
    }

    const float width = static_cast<float>(canvas->width());
    place(*bar, glm::vec2(0.0f, 0.0f), glm::vec2(width, dressing_.barHeight));

    canvas->rect(glm::vec2(0.0f, 0.0f), glm::vec2(width, dressing_.barHeight), dressing_.panel);
    // a rule along the bottom edge, so the strip reads as something over the scene rather
    // than as part of it
    canvas->rect(glm::vec2(0.0f, dressing_.barHeight), glm::vec2(width, dressing_.barHeight + ruleWidth), dressing_.border);

    float pen = dressing_.padding * 0.5f;
    for (std::size_t index = 0; index < bar->size(); index++) {
        const boost::shared_ptr<component::Menu> menu = bar->menu(index);
        if (!menu) {
            continue;
        }
        const std::string& label = bar->label(index);
        const float extent = measure_(label) + dressing_.padding;

        // on the bar rather than on the menu, whose own bounds are the panel it drops
        bar->place(index, glm::vec2(pen, 0.0f), glm::vec2(extent, dressing_.barHeight));

        const bool lit = bar->open() == static_cast<int>(index) || bar->hover() == static_cast<int>(index);
        if (lit) {
            canvas->rect(glm::vec2(pen, 0.0f), glm::vec2(pen + extent, dressing_.barHeight), dressing_.highlight);
        }
        write_(label, glm::vec2(pen + dressing_.padding * 0.5f, dressing_.barHeight * 0.7f), lit ? dressing_.activeText : dressing_.text);
        pen += extent;
    }

    // outermost first, so that a flyout is drawn over the panel it came out of
    const std::vector<boost::shared_ptr<component::Menu>>& panels = bar->panels();
    for (std::size_t depth = 0; depth < panels.size(); depth++) {
        glm::vec2 origin;
        if (depth == 0) {
            origin = glm::vec2(bar->bound(static_cast<std::size_t>(bar->open())).position().x, dressing_.barHeight);
        } else {
            // out of the right hand edge of the parent, level with the item it came from
            const v3d::type::Bound2D bounds = panels[depth - 1]->bound();
            const boost::shared_ptr<component::MenuItem> item = panels[depth - 1]->active();
            origin = glm::vec2(bounds.position().x + bounds.size().x,
                item ? item->position().y : bounds.position().y);
        }
        panel(canvas, panels[depth], origin);
    }
}

/**
 **/
float ComponentRenderer::widest(const component::Toolbar& bar) const {
    float widest = 0.0f;
    for (std::size_t index = 0; index < bar.size(); index++) {
        const boost::shared_ptr<component::Button> button = bar.button(index);
        if (button) {
            widest = std::max(widest, extent(*button));
        }
    }
    return widest;
}

/**
 **/
void ComponentRenderer::draw(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::Toolbar>& bar,
    const glm::vec2& corner) const {
    if (canvas == nullptr || !bar) {
        return;
    }

    const bool row = bar->edge() == component::Toolbar::Edge::Top;
    // a row spans the canvas and a column spans what is under the strips above it, so
    // that the rule along a strip's far edge runs the whole way
    const glm::vec2 size = row
        ? glm::vec2(static_cast<float>(canvas->width()) - corner.x, dressing_.barHeight)
        : glm::vec2(widest(*bar) + dressing_.padding, static_cast<float>(canvas->height()) - corner.y);

    place(*bar, corner, size);

    canvas->rect(corner, corner + size, dressing_.panel);
    if (row) {
        canvas->rect(glm::vec2(corner.x, corner.y + size.y),
            glm::vec2(corner.x + size.x, corner.y + size.y + ruleWidth), dressing_.border);
    } else {
        canvas->rect(glm::vec2(corner.x + size.x, corner.y),
            glm::vec2(corner.x + size.x + ruleWidth, corner.y + size.y), dressing_.border);
    }

    glm::vec2 pen = corner;
    for (std::size_t index = 0; index < bar->size(); index++) {
        const boost::shared_ptr<component::Button> button = bar->button(index);
        if (!button) {
            continue;
        }
        // the strip decides how big a button in it is - a row's is as wide as its label
        // and a column's is as wide as the strip - and the button is then drawn at the
        // size it was given, the same way a button anywhere else is
        const glm::vec2 box = row
            ? glm::vec2(extent(*button) + dressing_.padding, size.y)
            : glm::vec2(size.x, dressing_.lineHeight);

        place(*button, pen, box);
        draw(canvas, button);

        pen += row ? glm::vec2(box.x, 0.0f) : glm::vec2(0.0f, box.y);
    }
}

/**
 **/
void ComponentRenderer::panel(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::Menu>& menu,
    const glm::vec2& origin) const {
    const std::size_t count = menu->size();
    if (count == 0) {
        return;
    }

    const float column = dressing_.lineHeight * markColumn;

    std::vector<std::string> labels;
    labels.reserve(count);
    float widest = 0.0f;
    for (std::size_t index = 0; index < count; index++) {
        const boost::shared_ptr<component::MenuItem>& item = (*menu)[index];
        const std::string label = item ? item->text() : std::string();
        widest = std::max(widest, measure_(label));
        labels.push_back(label);
    }

    // a column either side of the labels: the mark on the left and the submenu arrow on
    // the right, both of which are there whether or not this menu uses them, so that
    // every label in one panel starts at the same place
    const glm::vec2 size(widest + column * 2.0f + dressing_.padding * 0.5f,
        dressing_.lineHeight * static_cast<float>(count) + dressing_.panelPadding * 2.0f);

    // a panel that would hang off an edge is moved back onto the canvas rather than
    // clipped, which is what puts the last menu of a bar's flyouts back inside the window
    glm::vec2 corner(
        std::min(origin.x, static_cast<float>(canvas->width()) - size.x),
        std::min(origin.y, static_cast<float>(canvas->height()) - size.y));
    corner = glm::vec2(std::max(corner.x, 0.0f), std::max(corner.y, 0.0f));

    place(*menu, corner, size);

    canvas->rect(corner - glm::vec2(1.0f, 1.0f), corner + size + glm::vec2(1.0f, 1.0f), dressing_.border);
    canvas->rect(corner, corner + size, dressing_.panel);

    const boost::shared_ptr<component::MenuItem> active = menu->active();

    for (std::size_t index = 0; index < count; index++) {
        const boost::shared_ptr<component::MenuItem>& item = (*menu)[index];
        const float top = corner.y + dressing_.panelPadding + dressing_.lineHeight * static_cast<float>(index);
        const bool selected = active && item == active;

        if (item) {
            place(*item, glm::vec2(corner.x, top), glm::vec2(size.x, dressing_.lineHeight));
        }

        if (selected) {
            canvas->rect(glm::vec2(corner.x, top), glm::vec2(corner.x + size.x, top + dressing_.lineHeight), dressing_.highlight);
        }

        if (item && item->checked()) {
            const glm::vec2 centre(corner.x + column * 0.5f, top + dressing_.lineHeight * 0.5f);
            const float mark = dressing_.lineHeight * 0.15f;
            canvas->rect(centre - glm::vec2(mark, mark), centre + glm::vec2(mark, mark), dressing_.activeText);
        }

        if (item && item->type() == component::menu::ItemType::Submenu) {
            // a three sided circle is a triangle with a vertex at zero degrees, which
            // points along +x
            canvas->circle(glm::vec2(corner.x + size.x - column * 0.5f, top + dressing_.lineHeight * 0.5f),
                dressing_.lineHeight * 0.18f, 3, selected ? dressing_.activeText : dressing_.text);
        }

        const glm::vec2 pen(corner.x + column, top + dressing_.lineHeight * 0.75f);
        write_(labels[index], pen, selected ? dressing_.activeText : dressing_.text);
    }
}

};  // namespace v3d::ui
