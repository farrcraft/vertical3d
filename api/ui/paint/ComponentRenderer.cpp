/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "ComponentRenderer.h"

#include <api/type/Checked.h>
#include <api/event/Event.h>
#include <api/type/geometry/Bound2D.h>
#include <api/ui/Component.h>
#include <api/ui/Container.h>
#include <api/ui/Interface.h>
#include <api/ui/Image.h>
#include <api/ui/Length.h>
#include <api/ui/Transition.h>
#include <api/ui/component/Bar.h>
#include <api/ui/component/Button.h>
#include <api/ui/component/CheckBox.h>
#include <api/ui/component/Icon.h>
#include <api/ui/component/Label.h>
#include <api/ui/component/Panel.h>
#include <api/ui/component/Scrollbar.h>
#include <api/ui/component/SelectList.h>
#include <api/ui/component/Slider.h>
#include <api/ui/component/TabBar.h>
#include <api/ui/component/TabPage.h>
#include <api/ui/component/TextBox.h>
#include <api/ui/component/Toolbar.h>
#include <api/ui/component/Type.h>
#include <api/ui/component/menu/Item.h>
#include <api/ui/component/menu/Menu.h>
#include <api/ui/component/menu/Bar.h>
#include <api/ui/style/Resolver.h>
#include <api/ui/style/Theme.h>
#include <api/ui/paint/internal/Easing.h>
#include <api/ui/paint/internal/MenuRenderer.h>
#include <api/ui/paint/internal/Skin.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "Dressing.h"
#include "Painter.h"
#include "Text.h"

namespace v3d::ui::paint {

namespace {

/**
 * The style class a component's focus ring is read from, or nothing for a component drawn
 * with the base dressing alone.
 *
 * Resolver::Class is not checked against Type, so this switch defines which type rings in
 * which class. Every enumerator is named and there is no default, so a component added to
 * Type is a build error here.
 **/
std::optional<style::Resolver::Class> ringed(component::Type type) noexcept {
    switch (type) {
        case component::Type::Panel:       return style::Resolver::Class::Panel;
        case component::Type::Bar:         return style::Resolver::Class::Bar;
        case component::Type::Scrollbar:   return style::Resolver::Class::Scrollbar;
        case component::Type::Slider:      return style::Resolver::Class::Slider;
        case component::Type::CheckBox:    return style::Resolver::Class::CheckBox;
        case component::Type::RadioButton: return style::Resolver::Class::Radio;
        case component::Type::SelectList:  return style::Resolver::Class::List;
        case component::Type::TabBar:
        case component::Type::TabPage:     return style::Resolver::Class::Tabs;
        case component::Type::TextBox:     return style::Resolver::Class::TextBox;
        case component::Type::Button:      return style::Resolver::Class::Button;
        case component::Type::HorizontalBox:
        case component::Type::Icon:
        case component::Type::Label:
        case component::Type::Menu:
        case component::Type::MenuBar:
        case component::Type::MenuItem:
        case component::Type::Toolbar:
        case component::Type::Undefined:
        case component::Type::VerticalBox:
            // a label, an icon, a box and the strips have no class of their own. They are
            // drawn and ringed with the base dressing
            break;
    }
    return std::nullopt;
}

};  // namespace

/**
 **/
ComponentRenderer::ComponentRenderer(const Measure& measure, const Write& write) :
    measure_(measure),
    write_(write),
    arranger_(measure, styles_),
    easing_(std::make_unique<internal::Easing>(styles_)),
    skin_(std::make_unique<internal::Skin>(styles_)),
    menus_(std::make_unique<internal::MenuRenderer>(measure, write, styles_, arranger_, *easing_)) {
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
void ComponentRenderer::scale(float factor) noexcept {
    styles_.scale(factor);
}

/**
 **/
void ComponentRenderer::clock(const Clock& now) noexcept {
    easing_->clock(now);
}

/**
 **/
const Dressing& ComponentRenderer::dressing() const noexcept {
    return base();
}

/**
 **/
void ComponentRenderer::shortcuts(const Shortcut& lookup) {
    menus_->shortcuts(lookup);
}

/**
 **/
std::string ComponentRenderer::shortcut(const v3d::event::Event& command) const {
    return menus_->shortcut(command);
}

/**
 **/
void ComponentRenderer::tooltip(v3d::render::realtime::canvas::Canvas* canvas, const std::string& text, const std::string& keys,
    const v3d::type::geometry::Bound2D& below, bool beside) const {
    if (canvas == nullptr || (text.empty() && keys.empty())) {
        return;
    }
    const float line = base().lineHeight;
    const float inset = line * tooltipInset;
    const float gap = keys.empty() || text.empty() ? 0.0F : inset;
    const glm::vec2 size(measure_(text) + gap + measure_(keys) + (inset * 2.0F), line);
    const glm::vec2 room(static_cast<float>(canvas->width()), static_cast<float>(canvas->height()));

    // just under the box, and back onto the canvas where it would leave it. Below the bottom
    // edge it goes above the box instead. Beside the box it goes right, or left past the right edge
    glm::vec2 corner(below.position().x, below.position().y + below.size().y + base().spacing);
    if (beside) {
        corner = glm::vec2(below.position().x + below.size().x + base().spacing, below.position().y + ((below.size().y - size.y) * 0.5F));
        if (corner.x + size.x > room.x) {
            corner.x = below.position().x - base().spacing - size.x;
        }
    } else if (corner.y + size.y > room.y) {
        corner.y = below.position().y - base().spacing - size.y;
    }
    corner = glm::vec2(std::clamp(corner.x, 0.0F, std::max(0.0F, room.x - size.x)), std::max(corner.y, 0.0F));

    menus_->cast(canvas, corner, corner + size);
    plateBox(canvas, corner, corner + size, base().radius, base().borderWidth, {.inside = base().panel, .outline = base().border});
    const float baseline = corner.y + (line * LINE_BASELINE);
    write_(text, glm::vec2(corner.x + inset, baseline), base().text);
    if (!keys.empty()) {
        write_(keys, glm::vec2(corner.x + size.x - inset - measure_(keys), baseline), base().secondaryText);
    }
}

/**
 **/
void ComponentRenderer::paint(v3d::render::realtime::canvas::Canvas* canvas,
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
        case component::Type::Slider:
            draw(canvas, boost::dynamic_pointer_cast<component::Slider>(component));
            break;
        case component::Type::CheckBox:
        case component::Type::RadioButton:
            // a radio button is a check box with a round mark, so one call draws both
            draw(canvas, boost::dynamic_pointer_cast<component::CheckBox>(component));
            break;
        case component::Type::SelectList:
            draw(canvas, boost::dynamic_pointer_cast<component::SelectList>(component));
            break;
        case component::Type::TextBox:
            draw(canvas, boost::dynamic_pointer_cast<component::TextBox>(component));
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
        case component::Type::HorizontalBox:
        case component::Type::VerticalBox:
        case component::Type::TabPage:
        case component::Type::Undefined:
        case component::Type::Menu:
        case component::Type::MenuBar:
        case component::Type::MenuItem:
        case component::Type::Toolbar:
            // types that draw nothing here. A box draws nothing of its own, and a tab page is a
            // box whose bar draws the strip above it. A menu and the strips are drawn by their
            // own draw overloads, which draw(Container) calls
            break;
    }
    ring(canvas, component);
}

/**
 **/
void ComponentRenderer::ring(v3d::render::realtime::canvas::Canvas* canvas,
    const boost::shared_ptr<Component>& component) const {
    // a component disabled while it held the focus draws no ring
    if (canvas == nullptr || !component->focused() || !usable(*component)) {
        return;
    }
    // the class the component is drawn in, so a theme can ring a text box differently from
    // a list. A component dressed by the base alone is ringed out of the base
    const std::optional<style::Resolver::Class> className = ringed(component->type());
    const Dressing& dress = className.has_value()
        ? styles_.resolve(*className, component->style()) : base();
    const float width = dress.focusWidth;
    if (width <= 0.0F || dress.focus.a <= 0.0F) {
        return;
    }
    // drawn here rather than in each component's draw, so every control shows the focus
    // the same way. Traced around the box it was drawn in, after that draw, so it sits
    // over the component rather than under whatever the component filled
    const glm::vec2 min = component->position();
    const glm::vec2 size = component->size();
    if (size.x <= 0.0F || size.y <= 0.0F) {
        return;  // never laid out, so there is no box to ring
    }
    strokeBox(canvas, min, min + size, dress.radius, width, dress.focus);
}

/**
 **/
void ComponentRenderer::draw(v3d::render::realtime::canvas::Canvas* canvas, const Interface& ui) const {
    for (const boost::shared_ptr<Container>& container : ui.containers()) {
        if (container && container->visible()) {
            draw(canvas, *container);
        }
    }
}

/**
 **/
void ComponentRenderer::draw(v3d::render::realtime::canvas::Canvas* canvas, const Container& container) const {
    if (canvas == nullptr) {
        return;
    }
    // the box every root component is laid out against
    const v3d::type::geometry::Bound2D area({.position = glm::vec2(0.0F, 0.0F), .size = glm::vec2(static_cast<float>(canvas->width()), static_cast<float>(canvas->height()))});
    if (container.backdrop()) {
        canvas->rect(area.position(), area.position() + area.size(), base().backdrop);
    }

    std::vector<std::pair<boost::shared_ptr<component::Toolbar>, glm::vec2>> strips;
    std::vector<boost::shared_ptr<component::menu::Bar>> bars;
    arranger_.stack(container, &strips, &bars);

    for (const boost::shared_ptr<Component>& component : container.ordered()) {
        if (!component || !component->visible()) {
            continue;
        }
        // the strips were placed by stack() and the menu bars are held back to the end
        if (component::traits(component->type()).strip) {
            continue;
        }
        if (component->type() == component::Type::Menu) {
            draw(canvas, boost::dynamic_pointer_cast<component::menu::Menu>(component));
        } else {
            // every other component is laid out against the canvas, and whatever it holds is
            // laid out against it
            arranger_.walk(canvas, component,
                component->layout().resolve(area, arranger_.natural(*component, area)),
                [this](v3d::render::realtime::canvas::Canvas* target, const boost::shared_ptr<Component>& each) {
                    paint(target, each);
                });
        }
    }

    for (const std::pair<boost::shared_ptr<component::Toolbar>, glm::vec2>& strip : strips) {
        draw(canvas, strip.first, strip.second);
    }
    // an open menu drops a panel over whatever the strips below it occupy, so a menu bar
    // is drawn after them
    for (const boost::shared_ptr<component::menu::Bar>& bar : bars) {
        draw(canvas, bar);
    }
}

/**
 **/
void ComponentRenderer::draw(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::Label>& label) const {
    if (canvas == nullptr || !label) {
        return;
    }
    const std::string text(label->text());
    const glm::vec2 size = arranger_.drawn(*label);
    Arranger::place(*label, {.position = label->position(), .size = size});

    glm::vec2 pen(label->position().x, label->position().y + (base().lineHeight * LINE_BASELINE));
    if (label->layout().width.unit() == Length::Unit::Auto) {
        write_(text, pen, ink(*label, base(), base().text));
        return;
    }
    // a label with a width to wrap to draws the rows the arranger sized it for, so the two
    // agree about how tall it is
    for (const std::string& row : wrap(text, size.x, measure_)) {
        write_(row, pen, ink(*label, base(), base().text));
        pen.y += base().lineHeight;
    }
}

/**
 **/
void ComponentRenderer::draw(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::Icon>& icon) const {
    if (canvas == nullptr || !icon || !icon->image().valid()) {
        return;
    }
    const glm::vec2 size = arranger_.drawn(*icon);
    Arranger::place(*icon, {.position = icon->position(), .size = size});

    const v3d::ui::Image& picture = icon->image();
    canvas->rect(icon->position(), icon->position() + size, picture.uv0, picture.uv1,
        ink(*icon, base(), glm::vec4(1.0F, 1.0F, 1.0F, 1.0F)), picture.texture);
}

/**
 **/
void ComponentRenderer::draw(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::Button>& button) const {
    if (canvas == nullptr || !button) {
        return;
    }
    const std::string label(button->label());

    const glm::vec2 size = arranger_.drawn(*button);
    const glm::vec2 min = button->position();
    Arranger::place(*button, {.position = min, .size = size});

    // a button that cannot be used is never lit, whatever the cursor last left on it. A
    // checked toggle keeps its highlight whether or not the cursor is on it, so a strip
    // shows which mask and which tool are in force. A held press shows over both
    const bool held = usable(*button) && button->state() == component::Button::STATE_PRESS;
    const bool lit = held || (usable(*button) &&
        (button->checked() || button->state() == component::Button::STATE_HOVER));
    if (!skin_->draw(canvas, *button, min, min + size)) {
        glm::vec4 face = button->checked() ? base().highlight : base().hover;
        if (held) {
            face = base().pressed;
        }
        const glm::vec4 shown = lit ? easing_->toward(&button->motion().face, face) : easing_->away(&button->motion().face);
        if (shown.a > 0.0F) {
            canvas->rect(min, min + size, shown);
        }
    }

    // a resolved icon is drawn instead of the label, not as well as it. Until the image has
    // been resolved, the button draws its label
    const v3d::ui::Image& picture = button->image();
    if (picture.valid()) {
        const float side = std::min(base().iconSize, std::min(size.x, size.y));
        const glm::vec2 corner = min + (size - glm::vec2(side, side)) * 0.5F;
        canvas->rect(corner, corner + glm::vec2(side, side), picture.uv0, picture.uv1,
            ink(*button, base(), glm::vec4(1.0F, 1.0F, 1.0F, 1.0F)), picture.texture);
        return;
    }

    const glm::vec2 baseline(min.x + ((size.x - measure_(label)) * 0.5F), min.y + (size.y * WIDGET_BASELINE));
    write_(label, baseline, easing_->toward(&button->motion().ink, ink(*button, base(), lit ? base().activeText : base().text)));
}

/**
 **/
void ComponentRenderer::draw(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::Panel>& panel) const {
    if (canvas == nullptr || !panel) {
        return;
    }
    const Dressing& dress = styles_.resolve(style::Resolver::Class::Panel, panel->style());
    const glm::vec2 min = panel->position();
    plateBox(canvas, min, min + panel->size(), dress.radius, dress.borderWidth, {.inside = dress.panel, .outline = dress.border});
}

/**
 **/
void ComponentRenderer::draw(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::Bar>& bar) const {
    if (canvas == nullptr || !bar) {
        return;
    }
    const Dressing& dress = styles_.resolve(style::Resolver::Class::Bar, bar->style());

    const glm::vec2 min = bar->position();
    const glm::vec2 max = min + bar->size();
    plateBox(canvas, min, max, dress.radius, dress.borderWidth, {.inside = dress.track, .outline = dress.border});
    if (bar->fraction() <= 0.0F) {
        return;
    }

    // the fill sits inside the border rather than under it, so a bar at full still reads
    // as something in a track
    const glm::vec2 inset(dress.borderWidth, dress.borderWidth);
    glm::vec2 low = min + inset;
    glm::vec2 high = max - inset;
    if (bar->direction() == component::Bar::Direction::Horizontal) {
        high.x = low.x + ((high.x - low.x) * bar->fraction());
    } else {
        // a vertical bar fills from the bottom, which is the way one is read
        low.y = high.y - ((high.y - low.y) * bar->fraction());
    }
    fillBox(canvas, low, high, std::max(0.0F, dress.radius - dress.borderWidth), dress.fill);
}

/**
 **/
void ComponentRenderer::draw(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::Scrollbar>& bar) const {
    if (canvas == nullptr || !bar) {
        return;
    }
    const Dressing& dress = styles_.resolve(style::Resolver::Class::Scrollbar, bar->style());

    const glm::vec2 min = bar->position();
    const glm::vec2 max = min + bar->size();
    plateBox(canvas, min, max, dress.radius, dress.borderWidth, {.inside = dress.track, .outline = dress.border});
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
    fillBox(canvas, low, high, std::max(0.0F, dress.radius - dress.borderWidth), dress.thumb);
}

/**
 **/
void ComponentRenderer::draw(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::Slider>& slider) const {
    if (canvas == nullptr || !slider) {
        return;
    }
    const Dressing& dress = styles_.resolve(style::Resolver::Class::Slider, slider->style());

    const glm::vec2 min = slider->position();
    const glm::vec2 max = min + slider->size();
    plateBox(canvas, min, max, dress.radius, dress.borderWidth, {.inside = dress.track, .outline = dress.border});

    // the thumb is a square as tall as the track. Its left edge travels from the start of
    // the track to the end less one side, so it never hangs off either end
    const float side = slider->size().y;
    const float travel = std::max(0.0F, slider->size().x - side);
    const float left = min.x + (travel * slider->fraction());
    const glm::vec2 inset(dress.borderWidth, dress.borderWidth);
    fillBox(canvas, min + inset, glm::vec2(left + (side * 0.5F), max.y - inset.y),
        std::max(0.0F, dress.radius - dress.borderWidth), dress.fill);
    fillBox(canvas, glm::vec2(left, min.y), glm::vec2(left + side, max.y), dress.radius, dress.thumb);
}

/**
 **/
void ComponentRenderer::draw(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::CheckBox>& box) const {
    if (canvas == nullptr || !box) {
        return;
    }
    const bool round = box->type() == component::Type::RadioButton;
    const std::string text(box->label());

    const glm::vec2 size = arranger_.drawn(*box);
    const glm::vec2 min = box->position();
    Arranger::place(*box, {.position = min, .size = size});

    const Dressing& dress = styles_.resolve(
        round ? style::Resolver::Class::Radio : style::Resolver::Class::CheckBox, box->style());
    const float side = std::min(dress.markSize, size.y);

    // the mark is centred in the row rather than sitting on its top edge, because the
    // label beside it is centred too
    const glm::vec2 corner(min.x, min.y + ((size.y - side) * 0.5F));
    if (round) {
        const glm::vec2 centre = corner + glm::vec2(side, side) * 0.5F;
        canvas->circle(centre, side * 0.5F, markSides, dress.border);
        canvas->circle(centre, (side * 0.5F) - dress.borderWidth, markSides, dress.track);
        if (box->checked()) {
            canvas->circle(centre, side * markFill * 0.5F, markSides, dress.mark);
        }
    } else {
        plateBox(canvas, corner, corner + glm::vec2(side, side), dress.radius, dress.borderWidth, {.inside = dress.track, .outline = dress.border});
        if (box->checked()) {
            const float inset = side * (1.0F - markFill) * 0.5F;
            fillBox(canvas, corner + glm::vec2(inset, inset), corner + glm::vec2(side - inset, side - inset),
                std::max(0.0F, dress.radius - dress.borderWidth), dress.mark);
        }
    }

    if (text.empty()) {
        return;
    }
    write_(text, glm::vec2(min.x + side + (dress.padding * 0.5F), min.y + (size.y * WIDGET_BASELINE)),
        ink(*box, dress, dress.text));
}

/**
 **/
void ComponentRenderer::draw(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::SelectList>& list) const {
    if (canvas == nullptr || !list) {
        return;
    }
    const glm::vec2 size = arranger_.drawn(*list);
    const glm::vec2 min = list->position();
    Arranger::place(*list, {.position = min, .size = size});

    const Dressing& dress = styles_.resolve(style::Resolver::Class::List, list->style());
    const float width = dress.borderWidth;
    const float row = dress.lineHeight;

    plateBox(canvas, min, min + size, dress.radius, width, {.inside = dress.panel, .outline = dress.border});

    // the row height comes from the style, and the list needs it to find the row under a
    // point, so it is stored on the list while drawing, as the list's box is
    list->rowHeight(row);
    if (list->items().empty() || row <= 0.0F) {
        return;
    }

    const glm::vec2 low(min.x + width, min.y + width);
    const glm::vec2 high(min.x + size.x - width, min.y + size.y - width);
    canvas->clip(low, high);

    const float scrolled = list->offset();
    // only the rows the box shows are drawn - a list of a thousand costs the rows on screen
    const std::size_t first = v3d::type::toCount(scrolled / row, std::numeric_limits<uint32_t>::max()).value_or(0U);
    const std::size_t last = std::min(list->items().size(),
        first + static_cast<std::size_t>(size.y / row) + 2U);

    for (std::size_t index = first; index < last; index++) {
        const float top = min.y + (static_cast<float>(index) * row) - scrolled;
        const bool picked = static_cast<int>(index) == list->selected();
        if (picked) {
            fillBox(canvas, glm::vec2(low.x, top), glm::vec2(high.x, top + row), 0.0F, dress.highlight);
        }
        write_(list->items()[index], glm::vec2(low.x + (dress.padding * 0.5F), top + (row * WIDGET_BASELINE)),
            ink(*list, dress, picked ? dress.activeText : dress.text));
    }

    canvas->unclip();
}

/**
 **/
void ComponentRenderer::draw(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::TextBox>& box) const {
    if (canvas == nullptr || !box) {
        return;
    }
    const glm::vec2 size = arranger_.drawn(*box);
    const glm::vec2 min = box->position();
    Arranger::place(*box, {.position = min, .size = size});

    const Dressing& dress = styles_.resolve(style::Resolver::Class::TextBox, box->style());
    const float width = dress.borderWidth;
    plateBox(canvas, min, min + size, dress.radius, width, {.inside = dress.panel, .outline = dress.border});

    const glm::vec2 low(min.x + width + (dress.padding * 0.5F), min.y + width);
    const glm::vec2 high(min.x + size.x - width, min.y + size.y - width);
    if (high.x <= low.x) {
        return;
    }
    canvas->clip(low, high);

    const std::string text(box->text());
    if (text.empty() && !box->focused()) {
        // the placeholder says what the box is for. It is shown only while the box is empty
        // and does not have the focus
        box->pen(low.x);
        write_(box->placeholder(), glm::vec2(low.x, min.y + (size.y * WIDGET_BASELINE)), dress.placeholder);
        canvas->unclip();
        return;
    }

    // where the caret falls in the line, which is also how far the line has to slide left
    // for the caret to stay inside the box
    const float caret = measure_(std::string_view(text).substr(0, box->caret()));
    const float room = high.x - low.x;
    const float slid = caret > room ? caret - room : 0.0F;

    const glm::vec2 pen(low.x - slid, min.y + (size.y * WIDGET_BASELINE));
    // left on the box so that a press can find the character under it, the way a list is
    // left holding the height of a row
    box->pen(pen.x);

    if (box->selected()) {
        // the highlight goes behind the line, and the line is still drawn whole in one
        // colour. Three separate runs could measure differently from the one prefix that
        // places the caret
        const std::string_view line(text);
        const float from = measure_(line.substr(0, std::min(box->anchor(), box->caret())));
        const float to = measure_(line.substr(0, std::max(box->anchor(), box->caret())));
        fillBox(canvas, glm::vec2(pen.x + from, low.y), glm::vec2(pen.x + to, high.y),
            0.0F, dress.highlight);
    }
    write_(text, pen, ink(*box, dress, dress.text));

    if (box->focused() && usable(*box)) {
        const float stem = std::max(width, 1.0F);
        canvas->rect(glm::vec2(pen.x + caret, low.y), glm::vec2(pen.x + caret + stem, high.y), dress.caret);
    }

    canvas->unclip();
}

/**
 **/
void ComponentRenderer::draw(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::TabBar>& bar) const {
    if (canvas == nullptr || !bar) {
        return;
    }
    const glm::vec2 size = arranger_.drawn(*bar);
    const glm::vec2 min = bar->position();
    Arranger::place(*bar, {.position = min, .size = size});

    const Dressing& dress = styles_.resolve(style::Resolver::Class::Tabs, bar->style());
    const float height = dress.barHeight;

    const std::vector<boost::shared_ptr<component::TabPage>> pages = bar->pages();
    std::vector<v3d::type::geometry::Bound2D> boxes;
    boxes.reserve(pages.size());

    float pen = min.x;
    for (std::size_t index = 0; index < pages.size(); index++) {
        const std::string label(pages[index]->label());
        const float width = measure_(label) + dress.padding;
        const glm::vec2 corner(pen, min.y);
        const glm::vec2 extent(width, height);
        boxes.push_back(v3d::type::geometry::Bound2D({.position = corner, .size = extent}));

        const bool picked = static_cast<int>(index) == bar->selected();
        fillBox(canvas, corner, corner + extent, dress.radius, picked ? dress.highlight : dress.tab);
        write_(label, glm::vec2(corner.x + (dress.padding * 0.5F), corner.y + (height * WIDGET_BASELINE)),
            ink(*bar, dress, picked ? dress.activeText : dress.text));
        pen += width + dress.borderWidth;
    }
    // where each tab ended up, for the cursor to be tested against
    bar->tabs(boxes);

    // the rule under the strip, which joins the chosen tab to the page below it
    canvas->rect(glm::vec2(min.x, min.y + height), glm::vec2(min.x + size.x, min.y + height + Arranger::ruleWidth), dress.border);

    if (!bar->page()) {
        return;
    }
    // the plate the chosen page sits on. The layout pass descends into the page, so
    // painting never reaches back into layout
    const v3d::type::geometry::Bound2D box = arranger_.page(*bar);
    fillBox(canvas, box.position(), box.position() + box.size(), 0.0F, dress.panel);
}

/**
 **/
glm::vec2 ComponentRenderer::insets(const Interface& ui) const {
    glm::vec2 total(0.0F, 0.0F);
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
void ComponentRenderer::draw(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::menu::Menu>& menu) const {
    menus_->draw(canvas, menu);
}

/**
 **/
void ComponentRenderer::draw(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::menu::Bar>& bar) const {
    menus_->draw(canvas, bar);
}

/**
 **/
void ComponentRenderer::draw(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::Toolbar>& bar,
    const glm::vec2& corner) const {
    if (canvas == nullptr || !bar) {
        return;
    }

    const bool row = bar->edge() == component::Toolbar::Edge::Top;
    arranger_.strip(*bar, corner, glm::vec2(static_cast<float>(canvas->width()), static_cast<float>(canvas->height())));
    const glm::vec2 size = bar->size();

    canvas->rect(corner, corner + size, base().panel);
    if (row) {
        canvas->rect(glm::vec2(corner.x, corner.y + size.y),
            glm::vec2(corner.x + size.x, corner.y + size.y + Arranger::ruleWidth), base().border);
    } else {
        canvas->rect(glm::vec2(corner.x + size.x, corner.y),
            glm::vec2(corner.x + size.x + Arranger::ruleWidth, corner.y + size.y), base().border);
    }

    for (std::size_t index = 0; index < bar->count(); index++) {
        const boost::shared_ptr<component::Button> button = bar->button(index);
        if (button && button->visible()) {
            draw(canvas, button);
        }
    }
}

glm::vec4 ComponentRenderer::ink(const Component& component, const Dressing& dress, const glm::vec4& colour) {
    return usable(component) ? colour : dress.disabledText;
}

};  // namespace v3d::ui::paint
