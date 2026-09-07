/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "ComponentRenderer.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
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
ComponentRenderer::ComponentRenderer(const Measure& measure, const Write& write) :
    measure_(measure),
    write_(write),
    arranger_(measure, styles_) {
}

// out of line, so that the header need not complete the types the members hold
ComponentRenderer::~ComponentRenderer() {
}

/**
 **/
Dressing& ComponentRenderer::dressing() noexcept {
    return styles_.base();
}

const Dressing& ComponentRenderer::base() const noexcept {
    return styles_.base();
}

/**
 **/
void ComponentRenderer::theme(const boost::shared_ptr<style::Theme>& theme) {
    styles_.theme(theme);
}

/**
 **/
boost::shared_ptr<style::Theme> ComponentRenderer::theme() const noexcept {
    return styles_.theme();
}

/**
 **/
void ComponentRenderer::paint(v3d::render::realtime::Canvas* canvas,
    const boost::shared_ptr<Component>& component) const {
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
            draw(canvas, boost::dynamic_pointer_cast<component::TabBar>(component));
            break;
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
    // the box every root component is laid out against
    const v3d::type::Bound2D area(glm::vec2(0.0f, 0.0f),
        glm::vec2(static_cast<float>(canvas->width()), static_cast<float>(canvas->height())));

    std::vector<std::pair<boost::shared_ptr<component::Toolbar>, glm::vec2>> strips;
    std::vector<boost::shared_ptr<component::MenuBar>> bars;
    arranger_.stack(container, &strips, &bars);

    for (const boost::shared_ptr<Component>& component : container.ordered()) {
        if (!component || !component->visible()) {
            continue;
        }
        // the strips were placed by stack() and the menu bars are held back to the end
        if (component->type() == component::Type::MenuBar ||
            component->type() == component::Type::Toolbar) {
            continue;
        }
        if (component->type() == component::Type::Menu) {
            draw(canvas, boost::dynamic_pointer_cast<component::Menu>(component));
        } else {
            // everything else is a box: it is laid out against the canvas, and whatever it
            // holds is laid out against it
            arranger_.walk(canvas, component,
                component->layout().resolve(area, arranger_.natural(*component), component->position()),
                [this](v3d::render::realtime::Canvas* target, const boost::shared_ptr<Component>& each) {
                    paint(target, each);
                });
        }
    }

    for (const std::pair<boost::shared_ptr<component::Toolbar>, glm::vec2>& strip : strips) {
        draw(canvas, strip.first, strip.second);
    }
    // an open menu drops a panel over whatever the strips below it occupy, so a menu bar
    // is drawn after them
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
        size = glm::vec2(measure_(text), base().lineHeight);
    }
    place(*label, label->position(), size);

    const glm::vec2 pen(label->position().x, label->position().y + base().lineHeight * 0.75f);
    write_(text, pen, base().text);
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
        size = glm::vec2(base().barHeight, base().barHeight);
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
        size = glm::vec2(arranger_.extent(*button) + base().padding, base().barHeight);
    }
    const glm::vec2 min = button->position();
    place(*button, min, size);

    // a checked toggle keeps its highlight whether or not the cursor is on it, which is
    // what says which mask and which tool are in force
    const bool lit = button->checked() || button->state() == component::Button::STATE_HOVER;
    if (!skin(canvas, *button, min, min + size) && lit) {
        canvas->rect(min, min + size, button->checked() ? base().highlight : base().hover);
    }

    // an icon is what the button says instead of its label, not as well as it. The label
    // stays on the component for whatever measures it before an image has been resolved
    if (button->texture().valid()) {
        const float side = std::min(base().iconSize, std::min(size.x, size.y));
        const glm::vec2 corner = min + (size - glm::vec2(side, side)) * 0.5f;
        canvas->rect(corner, corner + glm::vec2(side, side),
            glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), button->texture());
        return;
    }

    const glm::vec2 baseline(min.x + (size.x - measure_(label)) * 0.5f, min.y + size.y * 0.7f);
    write_(label, baseline, lit ? base().activeText : base().text);
}

/**
 **/
void ComponentRenderer::draw(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::Panel>& panel) const {
    if (canvas == nullptr || !panel) {
        return;
    }
    const Dressing& dress = styles_.resolve(style::Resolver::Class::Panel, panel->style());
    const glm::vec2 min = panel->position();
    plateBox(canvas, min, min + panel->size(), dress.radius, dress.borderWidth, dress.panel, dress.border);
}

/**
 **/
void ComponentRenderer::draw(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::Bar>& bar) const {
    if (canvas == nullptr || !bar) {
        return;
    }
    const Dressing& dress = styles_.resolve(style::Resolver::Class::Bar, bar->style());

    const glm::vec2 min = bar->position();
    const glm::vec2 max = min + bar->size();
    plateBox(canvas, min, max, dress.radius, dress.borderWidth, dress.track, dress.border);
    if (bar->fraction() <= 0.0f) {
        return;
    }

    // the fill sits inside the border rather than under it, so a bar at full still reads
    // as something in a track
    const glm::vec2 inset(dress.borderWidth, dress.borderWidth);
    glm::vec2 low = min + inset;
    glm::vec2 high = max - inset;
    if (bar->direction() == component::Bar::Direction::Horizontal) {
        high.x = low.x + (high.x - low.x) * bar->fraction();
    } else {
        // a vertical bar fills from the bottom, which is the way one is read
        low.y = high.y - (high.y - low.y) * bar->fraction();
    }
    fillBox(canvas, low, high, std::max(0.0f, dress.radius - dress.borderWidth), dress.fill);
}

/**
 **/
void ComponentRenderer::draw(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::Scrollbar>& bar) const {
    if (canvas == nullptr || !bar) {
        return;
    }
    const Dressing& dress = styles_.resolve(style::Resolver::Class::Scrollbar, bar->style());

    const glm::vec2 min = bar->position();
    const glm::vec2 max = min + bar->size();
    plateBox(canvas, min, max, dress.radius, dress.borderWidth, dress.track, dress.border);
    if (!bar->scrollable()) {
        // a page showing all of its content has a thumb the length of the track, which
        // would read as a bar scrolled nowhere rather than as one with nowhere to go
        return;
    }

    // the thumb sits inside the border, the way a bar's fill does
    const glm::vec2 inset(dress.borderWidth, dress.borderWidth);
    glm::vec2 low = min + inset;
    glm::vec2 high = max - inset;
    if (bar->direction() == component::Scrollbar::Direction::Vertical) {
        low.y = min.y + bar->thumbStart();
        high.y = low.y + bar->thumb();
    } else {
        low.x = min.x + bar->thumbStart();
        high.x = low.x + bar->thumb();
    }
    fillBox(canvas, low, high, std::max(0.0f, dress.radius - dress.borderWidth), dress.thumb);
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
        size = arranger_.natural(*box);
    }
    const glm::vec2 min = box->position();
    place(*box, min, size);

    const Dressing& dress = styles_.resolve(
        round ? style::Resolver::Class::Radio : style::Resolver::Class::CheckBox, box->style());
    const float side = std::min(dress.markSize, size.y);

    // the mark is centred in the row rather than sitting on its top edge, because the
    // label beside it is centred too
    const glm::vec2 corner(min.x, min.y + (size.y - side) * 0.5f);
    if (round) {
        const glm::vec2 centre = corner + glm::vec2(side, side) * 0.5f;
        canvas->circle(centre, side * 0.5f, markSides, dress.border);
        canvas->circle(centre, side * 0.5f - dress.borderWidth, markSides, dress.track);
        if (box->checked()) {
            canvas->circle(centre, side * markFill * 0.5f, markSides, dress.mark);
        }
    } else {
        plateBox(canvas, corner, corner + glm::vec2(side, side), dress.radius, dress.borderWidth,
            dress.track, dress.border);
        if (box->checked()) {
            const float inset = side * (1.0f - markFill) * 0.5f;
            fillBox(canvas, corner + glm::vec2(inset, inset), corner + glm::vec2(side - inset, side - inset),
                std::max(0.0f, dress.radius - dress.borderWidth), dress.mark);
        }
    }

    if (text.empty()) {
        return;
    }
    write_(text, glm::vec2(min.x + side + dress.padding * 0.5f, min.y + size.y * 0.7f), dress.text);
}

/**
 **/
void ComponentRenderer::draw(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::SelectList>& list) const {
    if (canvas == nullptr || !list) {
        return;
    }
    glm::vec2 size = list->size();
    if (size.x <= 0.0f || size.y <= 0.0f) {
        size = arranger_.natural(*list);
    }
    const glm::vec2 min = list->position();
    place(*list, min, size);

    const Dressing& dress = styles_.resolve(style::Resolver::Class::List, list->style());
    const float width = dress.borderWidth;
    const float row = dress.lineHeight;

    plateBox(canvas, min, min + size, dress.radius, width, dress.panel, dress.border);

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
            fillBox(canvas, glm::vec2(low.x, top), glm::vec2(high.x, top + row), 0.0f, dress.highlight);
        }
        write_(list->items()[index], glm::vec2(low.x + dress.padding * 0.5f, top + row * 0.7f),
            picked ? dress.activeText : dress.text);
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
        size = arranger_.natural(*bar);
    }
    const glm::vec2 min = bar->position();
    place(*bar, min, size);

    const Dressing& dress = styles_.resolve(style::Resolver::Class::Tabs, bar->style());
    const float height = dress.barHeight;

    const std::vector<boost::shared_ptr<component::TabPage>> pages = bar->pages();
    std::vector<v3d::type::Bound2D> boxes;
    boxes.reserve(pages.size());

    float pen = min.x;
    for (std::size_t index = 0; index < pages.size(); index++) {
        const std::string label(pages[index]->label());
        const float width = measure_(label) + dress.padding;
        const glm::vec2 corner(pen, min.y);
        const glm::vec2 extent(width, height);
        boxes.push_back(v3d::type::Bound2D(corner, extent));

        const bool picked = static_cast<int>(index) == bar->selected();
        fillBox(canvas, corner, corner + extent, dress.radius, picked ? dress.highlight : dress.track);
        write_(label, glm::vec2(corner.x + dress.padding * 0.5f, corner.y + height * 0.7f),
            picked ? dress.activeText : dress.text);
        pen += width + dress.borderWidth;
    }
    // where each tab ended up, for the cursor to be tested against - the same rule as a
    // component's own box, per ADR-0019
    bar->tabs(boxes);

    // the rule under the strip, which is what joins the chosen tab to the page below it
    canvas->rect(glm::vec2(min.x, min.y + height), glm::vec2(min.x + size.x, min.y + height + Arranger::ruleWidth), dress.border);

    if (!bar->page()) {
        return;
    }
    // the plate the chosen page sits on. Descending into the page is the walk's, so that
    // painting never reaches back into layout
    const v3d::type::Bound2D box = arranger_.page(*bar);
    fillBox(canvas, box.position(), box.position() + box.size(), 0.0f, dress.panel);
}

/**
 **/
bool ComponentRenderer::skin(v3d::render::realtime::Canvas* canvas, const component::Button& button,
    const glm::vec2& min, const glm::vec2& max) const {
    const boost::shared_ptr<style::Theme> theme = styles_.theme();
    if (!theme) {
        return false;
    }

    // a button's styles are told apart by state as well as by name, so the set is walked
    // rather than asked for one
    boost::shared_ptr<v3d::ui::Style> target;
    for (const boost::shared_ptr<v3d::ui::Style>& candidate : theme->getStyleSet(std::string(button.style()), "button")) {
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
    return arranger_.stack(container, nullptr, nullptr);
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

    const float width = widest + base().padding * 2.0f;
    const float height = base().lineHeight * static_cast<float>(count) + base().padding * 2.0f;
    const glm::vec2 origin(
        (static_cast<float>(canvas->width()) - width) * 0.5f,
        (static_cast<float>(canvas->height()) - height) * 0.5f);

    // a one pixel border, as a filled rectangle with the panel drawn over it
    canvas->rect(origin - glm::vec2(1.0f, 1.0f), origin + glm::vec2(width + 1.0f, height + 1.0f), base().border);
    canvas->rect(origin, origin + glm::vec2(width, height), base().panel);

    const boost::shared_ptr<component::MenuItem> active = level->active();

    for (std::size_t index = 0; index < count; index++) {
        const float top = origin.y + base().padding + base().lineHeight * static_cast<float>(index);
        const bool selected = active && (*level)[index] == active;

        if (selected) {
            canvas->rect(
                glm::vec2(origin.x + base().padding * 0.5f, top),
                glm::vec2(origin.x + width - base().padding * 0.5f, top + base().lineHeight),
                base().highlight);
        }

        // the pen sits on the baseline, which is most of the way down the line box - the
        // remainder is where descenders go
        const glm::vec2 pen(origin.x + base().padding, top + base().lineHeight * 0.75f);
        write_(labels[index], pen, selected ? base().activeText : base().text);
    }
}

/**
 **/
void ComponentRenderer::draw(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::MenuBar>& bar) const {
    if (canvas == nullptr || !bar) {
        return;
    }

    const float width = static_cast<float>(canvas->width());
    place(*bar, glm::vec2(0.0f, 0.0f), glm::vec2(width, base().barHeight));

    canvas->rect(glm::vec2(0.0f, 0.0f), glm::vec2(width, base().barHeight), base().panel);
    // a rule along the bottom edge, so the strip reads as something over the scene rather
    // than as part of it
    canvas->rect(glm::vec2(0.0f, base().barHeight), glm::vec2(width, base().barHeight + Arranger::ruleWidth), base().border);

    float pen = base().padding * 0.5f;
    for (std::size_t index = 0; index < bar->size(); index++) {
        const boost::shared_ptr<component::Menu> menu = bar->menu(index);
        if (!menu) {
            continue;
        }
        const std::string& label = bar->label(index);
        const float extent = measure_(label) + base().padding;

        // on the bar rather than on the menu, whose own bounds are the panel it drops
        bar->place(index, glm::vec2(pen, 0.0f), glm::vec2(extent, base().barHeight));

        const bool lit = bar->open() == static_cast<int>(index) || bar->hover() == static_cast<int>(index);
        if (lit) {
            canvas->rect(glm::vec2(pen, 0.0f), glm::vec2(pen + extent, base().barHeight), base().highlight);
        }
        write_(label, glm::vec2(pen + base().padding * 0.5f, base().barHeight * 0.7f), lit ? base().activeText : base().text);
        pen += extent;
    }

    // outermost first, so that a flyout is drawn over the panel it came out of
    const std::vector<boost::shared_ptr<component::Menu>>& panels = bar->panels();
    for (std::size_t depth = 0; depth < panels.size(); depth++) {
        glm::vec2 origin;
        if (depth == 0) {
            origin = glm::vec2(bar->bound(static_cast<std::size_t>(bar->open())).position().x, base().barHeight);
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
void ComponentRenderer::draw(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::Toolbar>& bar,
    const glm::vec2& corner) const {
    if (canvas == nullptr || !bar) {
        return;
    }

    const bool row = bar->edge() == component::Toolbar::Edge::Top;
    // a row spans the canvas and a column spans what is under the strips above it, so
    // that the rule along a strip's far edge runs the whole way
    const glm::vec2 size = row
        ? glm::vec2(static_cast<float>(canvas->width()) - corner.x, base().barHeight)
        : glm::vec2(arranger_.widest(*bar) + base().padding, static_cast<float>(canvas->height()) - corner.y);

    place(*bar, corner, size);

    canvas->rect(corner, corner + size, base().panel);
    if (row) {
        canvas->rect(glm::vec2(corner.x, corner.y + size.y),
            glm::vec2(corner.x + size.x, corner.y + size.y + Arranger::ruleWidth), base().border);
    } else {
        canvas->rect(glm::vec2(corner.x + size.x, corner.y),
            glm::vec2(corner.x + size.x + Arranger::ruleWidth, corner.y + size.y), base().border);
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
            ? glm::vec2(arranger_.extent(*button) + base().padding, size.y)
            : glm::vec2(size.x, base().lineHeight);

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

    const float column = base().lineHeight * markColumn;

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
    const glm::vec2 size(widest + column * 2.0f + base().padding * 0.5f,
        base().lineHeight * static_cast<float>(count) + base().panelPadding * 2.0f);

    // a panel that would hang off an edge is moved back onto the canvas rather than
    // clipped, which is what puts the last menu of a bar's flyouts back inside the window
    glm::vec2 corner(
        std::min(origin.x, static_cast<float>(canvas->width()) - size.x),
        std::min(origin.y, static_cast<float>(canvas->height()) - size.y));
    corner = glm::vec2(std::max(corner.x, 0.0f), std::max(corner.y, 0.0f));

    place(*menu, corner, size);

    canvas->rect(corner - glm::vec2(1.0f, 1.0f), corner + size + glm::vec2(1.0f, 1.0f), base().border);
    canvas->rect(corner, corner + size, base().panel);

    const boost::shared_ptr<component::MenuItem> active = menu->active();

    for (std::size_t index = 0; index < count; index++) {
        const boost::shared_ptr<component::MenuItem>& item = (*menu)[index];
        const float top = corner.y + base().panelPadding + base().lineHeight * static_cast<float>(index);
        const bool selected = active && item == active;

        if (item) {
            place(*item, glm::vec2(corner.x, top), glm::vec2(size.x, base().lineHeight));
        }

        if (selected) {
            canvas->rect(glm::vec2(corner.x, top), glm::vec2(corner.x + size.x, top + base().lineHeight), base().highlight);
        }

        if (item && item->checked()) {
            const glm::vec2 centre(corner.x + column * 0.5f, top + base().lineHeight * 0.5f);
            const float mark = base().lineHeight * 0.15f;
            canvas->rect(centre - glm::vec2(mark, mark), centre + glm::vec2(mark, mark), base().activeText);
        }

        if (item && item->type() == component::menu::ItemType::Submenu) {
            // a three sided circle is a triangle with a vertex at zero degrees, which
            // points along +x
            canvas->circle(glm::vec2(corner.x + size.x - column * 0.5f, top + base().lineHeight * 0.5f),
                base().lineHeight * 0.18f, 3, selected ? base().activeText : base().text);
        }

        const glm::vec2 pen(corner.x + column, top + base().lineHeight * 0.75f);
        write_(labels[index], pen, selected ? base().activeText : base().text);
    }
}

};  // namespace v3d::ui
