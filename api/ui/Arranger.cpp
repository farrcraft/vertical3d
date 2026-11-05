/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Arranger.h"

#include <api/render/realtime/canvas/Canvas.h>
#include <api/type/geometry/Bound2D.h>
#include <api/ui/DrawOrder.h>
#include <api/ui/component/Box.h>
#include <api/ui/component/Button.h>
#include <api/ui/component/CheckBox.h>
#include <api/ui/component/Label.h>
#include <api/ui/component/Scrollbar.h>
#include <api/ui/component/SelectList.h>
#include <api/ui/component/TabBar.h>
#include <api/ui/component/Toolbar.h>
#include <api/ui/component/Type.h>
#include <api/ui/component/menu/Menu.h>
#include <api/ui/component/menu/Bar.h>
#include <api/ui/component/menu/Item.h>
#include <api/ui/paint/Text.h>
#include <api/ui/paint/Type.h>
#include <api/ui/style/Resolver.h>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <glm/common.hpp>
#include <glm/vec2.hpp>

#include "Component.h"
#include "Container.h"
#include "Length.h"

namespace v3d::ui {

const float Arranger::ruleWidth = 1.0F;

Arranger::Arranger(const paint::Type& body, const style::Resolver& styles) :
    type_(body),
    styles_(styles),
    serial_(nextSerial()) {
}

/**
 **/
void Arranger::walk(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<Component>& component,
    const v3d::type::geometry::Bound2D& bounds, const Paint& paint) const {
    if (!component || !component->visible()) {
        return;
    }
    place(*component, {.position = bounds.position(), .size = bounds.size()});

    if (paint) {
        paint(canvas, component);
    }

    const std::vector<boost::shared_ptr<Component>>& children = component->children();
    if (children.empty()) {
        return;
    }

    // a component with clip() set clips what it holds to its own box. A clip is pushed onto
    // the canvas, so a layout pass with no canvas clips nothing
    const bool cut = component->clip() && canvas != nullptr;
    if (cut) {
        canvas->clip(component->position(), component->position() + component->size());
    }

    // forEachDrawn decides which children are visited, and in what order. This loop decides
    // each child's box: a tab bar's chosen page fills the room under the strip, a flow box's
    // children take the boxes arrange() wrote, and any other child resolves its own layout
    const component::Traits kind = component::traits(component->type());
    const auto* tabs = kind.pages ? static_cast<const component::TabBar*>(component.get()) : nullptr;
    const auto* box = kind.flow ? static_cast<const component::Box*>(component.get()) : nullptr;
    std::vector<v3d::type::geometry::Bound2D> boxes;
    if (box != nullptr) {
        boxes.reserve(children.size());
        arrange(*box, component->bound(), &boxes);
    }
    const v3d::type::geometry::Bound2D room = component->bound();
    std::size_t index = 0;
    forEachDrawn(*component, [&](const boost::shared_ptr<Component>& child) {
        if (tabs != nullptr) {
            walk(canvas, child, page(*tabs), paint);
        } else if (box != nullptr) {
            walk(canvas, child, boxes[index++], paint);
        } else {
            walk(canvas, child, child->layout().resolve(room, natural(*child, room)), paint);
        }
    });

    if (cut) {
        canvas->unclip();
    }
}

/**
 **/
glm::vec2 Arranger::natural(Component& component, const v3d::type::geometry::Bound2D& room) const {
    switch (component.type()) {
        case component::Type::Label: {
            const auto* label = dynamic_cast<const component::Label*>(&component);
            return label == nullptr ? glm::vec2(0.0F, 0.0F) : labelSize(*label, room);
        }
        case component::Type::Icon:
            // an icon given no size is a square the height of a strip
            return glm::vec2(styles_.base().barHeight, styles_.base().barHeight);
        case component::Type::Button: {
            const auto* button = dynamic_cast<const component::Button*>(&component);
            return button == nullptr ? glm::vec2(0.0F, 0.0F)
                : glm::vec2(extent(*button) + styles_.base().padding, styles_.base().barHeight);
        }
        case component::Type::CheckBox:
        case component::Type::RadioButton: {
            const auto* box = dynamic_cast<const component::CheckBox*>(&component);
            return box == nullptr ? glm::vec2(0.0F, 0.0F) : markSize(*box);
        }
        case component::Type::SelectList: {
            auto* list = dynamic_cast<component::SelectList*>(&component);
            return list == nullptr ? glm::vec2(0.0F, 0.0F) : listSize(list, room);
        }
        case component::Type::TextBox: {
            // a text box is as wide as the room it is in and as tall as one line. Its text
            // does not size it, so it does not grow as it is typed into
            const paint::Dressing& dress = styles_.resolve(style::Resolver::Class::TextBox, component.style());
            return glm::vec2(room.size().x, dress.lineHeight + dress.padding);
        }
        case component::Type::Slider:
            // a slider runs the width it is given and is as tall as the thumb its class names
            return glm::vec2(room.size().x,
                styles_.resolve(style::Resolver::Class::Slider, component.style()).markSize);
        case component::Type::Scrollbar: {
            // a scrollbar's natural size sets only its thickness. Its length is the room it
            // is offered
            const auto* bar = dynamic_cast<const component::Scrollbar*>(&component);
            if (bar == nullptr) {
                return glm::vec2(0.0F, 0.0F);
            }
            return bar->direction() == component::Scrollbar::Direction::Vertical
                ? glm::vec2(styles_.base().scrollbarWidth, room.size().y)
                : glm::vec2(room.size().x, styles_.base().scrollbarWidth);
        }
        case component::Type::HorizontalBox:
        case component::Type::VerticalBox:
            return box(component, room);
        case component::Type::Bar:
        case component::Type::Menu:
        case component::Type::MenuBar:
        case component::Type::MenuItem:
        case component::Type::Panel:
        case component::Type::TabBar:
        case component::Type::TabPage:
        case component::Type::Toolbar:
        case component::Type::Undefined:
            // these have no natural size, so an Auto extent on one is the room it is in
            break;
    }
    // every enumerator is handled above and the switch carries no default, so C4062 names
    // this function when a component type is added
    return room.size();
}

/**
 **/
glm::vec2 Arranger::labelSize(const component::Label& label, const v3d::type::geometry::Bound2D& room) const {
    const float line = type_.width(label.text());
    // a label given a width wraps to it. Its natural height is then the height of the
    // rows it wraps into, which an Auto height resolves to
    if (label.layout().width.unit() == Length::Unit::Auto) {
        return glm::vec2(line, styles_.base().lineHeight);
    }
    const float width = label.layout().width.resolve({.extent = room.size().x, .own = line});
    const std::size_t rows = type_.rows(label.text(), width).size();
    return glm::vec2(line,
        styles_.base().lineHeight * static_cast<float>(std::max<std::size_t>(rows, 1)));
}

/**
 **/
glm::vec2 Arranger::markSize(const component::CheckBox& box) const {
    // measured with its own style class, because the mark is drawn at the size that
    // class sets, which can differ from the base
    const paint::Dressing& dress = styles_.resolve(
        box.type() == component::Type::RadioButton
            ? style::Resolver::Class::Radio : style::Resolver::Class::CheckBox,
        box.style());
    const float text = box.label().empty() ? 0.0F
        : (dress.padding * 0.5F) + type_.width(box.label());
    return glm::vec2(dress.markSize + text, std::max(dress.markSize, dress.lineHeight));
}

/**
 **/
glm::vec2 Arranger::listSize(component::SelectList* list, const v3d::type::geometry::Bound2D& room) const {
    // this measures every row, and the result changes only when the rows or the
    // measure do. The list caches it against this arranger's serial, because an
    // arranger holds one measure, and drops it when it is given new rows
    float widest = list->widest(serial_);
    if (widest < 0.0F) {
        widest = 0.0F;
        for (const std::string& item : list->items()) {
            widest = std::max(widest, type_.width(item));
        }
        list->widest({.width = widest, .measure = serial_});
    }
    return glm::vec2(
        widest + styles_.resolve(style::Resolver::Class::List, list->style()).padding,
        room.size().y);
}

/**
 **/
void Arranger::arrange(const component::Box& box, const v3d::type::geometry::Bound2D& bounds,
    std::vector<v3d::type::geometry::Bound2D>* boxes) const {
    if (box.wrap()) {
        wrapped(box, bounds, boxes);
        return;
    }
    const bool vertical = box.type() == component::Type::VerticalBox;
    const glm::vec2 extent = bounds.size();
    float pen = vertical ? bounds.position().y : bounds.position().x;

    const v3d::type::geometry::Bound2D room = lineRoom(vertical, bounds);

    for (const boost::shared_ptr<Component>& child : box.children()) {
        if (!child || !child->visible()) {
            // a hidden child leaves no gap, so the visible ones close up
            boxes->push_back(v3d::type::geometry::Bound2D({.position = bounds.position(), .size = glm::vec2(0.0F, 0.0F)}));
            continue;
        }
        const Layout& layout = child->layout();
        glm::vec2 size = childSize(*child, room, extent);
        glm::vec2 corner;
        if (vertical) {
            if (box.stretch()) {
                size.x = extent.x;
            }
            corner = glm::vec2(bounds.position().x + layout.x.resolve({.extent = extent.x, .own = 0.0F}), pen);
            pen += size.y + box.spacing();
        } else {
            if (box.stretch()) {
                size.y = extent.y;
            }
            corner = glm::vec2(pen, bounds.position().y + layout.y.resolve({.extent = extent.y, .own = 0.0F}));
            pen += size.x + box.spacing();
        }
        boxes->push_back(v3d::type::geometry::Bound2D({.position = corner, .size = size}));
    }
}

/**
 **/
glm::vec2 Arranger::box(const Component& component, const v3d::type::geometry::Bound2D& room) const {
    // a box that wraps is as long as the line it is given, and as deep as the lines its
    // children fill. A box that does not wrap has no natural size, like a panel
    const auto* flow = component::traits(component.type()).flow ? static_cast<const component::Box*>(&component) : nullptr;
    if (flow == nullptr || !flow->wrap()) {
        return room.size();
    }
    const bool vertical = component.type() == component::Type::VerticalBox;
    const Layout& layout = component.layout();
    const glm::vec2 line(layout.width.resolve({.extent = room.size().x, .own = room.size().x}),
        layout.height.resolve({.extent = room.size().y, .own = room.size().y}));
    const float across = wrapped(*flow, v3d::type::geometry::Bound2D({.position = room.position(), .size = line}), nullptr);
    return vertical ? glm::vec2(across, line.y) : glm::vec2(line.x, across);
}

/**
 **/
float Arranger::wrapped(const component::Box& box, const v3d::type::geometry::Bound2D& bounds,
    std::vector<v3d::type::geometry::Bound2D>* boxes) const {
    const bool vertical = box.type() == component::Type::VerticalBox;
    const glm::vec2 extent = bounds.size();
    // the axis the line runs along, and the one the lines stack across
    const int along = vertical ? 1 : 0;
    const int across = vertical ? 0 : 1;
    const float start = bounds.position()[along];
    const float end = start + extent[along];

    const v3d::type::geometry::Bound2D room = lineRoom(vertical, bounds);

    float pen = start;
    float line = bounds.position()[across];
    float deepest = 0.0F;
    bool placed = false;
    for (const boost::shared_ptr<Component>& child : box.children()) {
        if (!child || !child->visible()) {
            if (boxes != nullptr) {
                boxes->push_back(v3d::type::geometry::Bound2D({.position = bounds.position(), .size = glm::vec2(0.0F, 0.0F)}));
            }
            continue;
        }
        const glm::vec2 size = childSize(*child, room, extent);

        // the first child on a line stays on it however long it is, so nothing is lost
        if (pen > start && pen + size[along] > end) {
            line += deepest + box.spacing();
            pen = start;
            deepest = 0.0F;
        }
        glm::vec2 corner;
        corner[along] = pen;
        corner[across] = line;
        if (boxes != nullptr) {
            boxes->push_back(v3d::type::geometry::Bound2D({.position = corner, .size = size}));
        }
        pen += size[along] + box.spacing();
        deepest = std::max(deepest, size[across]);
        placed = true;
    }
    return placed ? line + deepest - bounds.position()[across] : 0.0F;
}

/**
 **/
glm::vec2 Arranger::stack(const Container& container,
    std::vector<std::pair<boost::shared_ptr<component::Toolbar>, glm::vec2>>* strips,
    std::vector<boost::shared_ptr<component::menu::Bar>>* bars) const {
    const float row = styles_.base().barHeight + ruleWidth;
    std::vector<boost::shared_ptr<component::Toolbar>> shown;
    // a docked menu bar is drawn at the top of the canvas, so every docked bar comes first. The
    // top strips go under them in the order they are listed. The left strips start below all of
    // those, whatever order they are listed in
    float menus = 0.0F;
    float tops = 0.0F;
    for (const boost::shared_ptr<Component>& component : container.ordered()) {
        if (!component || !component->visible() || !component::traits(component->type()).strip) {
            continue;
        }
        if (component->type() == component::Type::MenuBar) {
            const boost::shared_ptr<component::menu::Bar> menuBar = boost::dynamic_pointer_cast<component::menu::Bar>(component);
            if (bars != nullptr) {
                bars->push_back(menuBar);
            }
            // a bar laid out by its Layout sits over the window rather than taking a strip
            if (menuBar->docked()) {
                menus += row;
            }
            continue;
        }
        const boost::shared_ptr<component::Toolbar> bar =
            boost::dynamic_pointer_cast<component::Toolbar>(component);
        if (!bar) {
            continue;
        }
        if (bar->edge() == component::Toolbar::Edge::Top) {
            tops += row;
        }
        shown.push_back(bar);
    }

    glm::vec2 taken(0.0F, menus);
    for (const boost::shared_ptr<component::Toolbar>& bar : shown) {
        const bool top = bar->edge() == component::Toolbar::Edge::Top;
        const glm::vec2 corner = top ? glm::vec2(0.0F, taken.y) : glm::vec2(taken.x, menus + tops);
        if (strips != nullptr) {
            strips->push_back(std::make_pair(bar, corner));
        }
        if (top) {
            taken.y += row;
        } else {
            // the width the strip will be drawn at, not the box it was last drawn in, which
            // is empty until it has been drawn once
            taken.x += widest(*bar) + styles_.base().padding + ruleWidth;
        }
    }
    taken.y = menus + tops;
    return taken;
}

/**
 **/
v3d::type::geometry::Bound2D Arranger::page(const component::TabBar& bar) const {
    const float height = styles_.resolve(style::Resolver::Class::Tabs, bar.style()).barHeight;
    const glm::vec2 min = bar.position();
    const glm::vec2 size = bar.size();
    return v3d::type::geometry::Bound2D({.position = glm::vec2(min.x, min.y + height + ruleWidth), .size = glm::vec2(size.x, std::max(size.y - height - ruleWidth, 0.0F))});
}

/**
 **/
float Arranger::extent(const component::Button& button) const {
    // sized by whether the button names an icon, not by whether the icon has been resolved,
    // because a strip is laid out before anything has been resolved
    if (!button.icon().empty()) {
        return styles_.base().iconSize;
    }
    // a tab is measured semibold, as it is set while chosen, so choosing it moves nothing
    if (button.toggle() && button.tab() != component::Button::Tab::None) {
        return type_.in(paint::weight(styles_.base().titleWeight)).width(button.label());
    }
    return type_.width(button.label());
}

/**
 **/
float Arranger::widest(const component::Toolbar& bar) const {
    float widest = 0.0F;
    for (std::size_t index = 0; index < bar.count(); index++) {
        const boost::shared_ptr<component::Button> button = bar.button(index);
        if (button && button->visible()) {
            widest = std::max(widest, extent(*button));
        }
    }
    return widest;
}

/**
 **/
void Arranger::place(Component& component, const v3d::type::geometry::Bound2D::Area& area) {
    const glm::vec2& position = area.position;
    const glm::vec2& size = area.size;
    component.position(position);
    component.size(size);
}

/**
 **/
glm::vec2 Arranger::drawn(Component& component) const {
    const glm::vec2 size = component.size();
    if (size.x > 0.0F && size.y > 0.0F) {
        return size;
    }
    return natural(component, component.bound());
}

/**
 **/
void Arranger::strip(component::Toolbar& bar, const glm::vec2& corner, const glm::vec2& room) const {
    const bool row = bar.edge() == component::Toolbar::Edge::Top;
    // a row spans the canvas and a column spans what is under the strips above it, so that
    // the rule along a strip's far edge runs the whole way
    const glm::vec2 size = row
        ? glm::vec2(room.x - corner.x, styles_.base().barHeight)
        : glm::vec2(widest(bar) + styles_.base().padding, room.y - corner.y);
    place(bar, {.position = corner, .size = size});

    glm::vec2 pen = corner;
    bool placed = false;
    for (std::size_t index = 0; index < bar.count(); index++) {
        const boost::shared_ptr<component::Button> button = bar.button(index);
        // a hidden button takes no room, so the ones after it close up
        if (!button || !button->visible()) {
            continue;
        }
        // a rule between two groups takes half a line, and one with no button before it none
        if (placed && bar.separated(index)) {
            const float gap = styles_.base().lineHeight * separatorShare;
            pen += row ? glm::vec2(gap, 0.0F) : glm::vec2(0.0F, gap);
        }
        placed = true;
        // the strip decides how big a button in it is. A row's is as wide as its label and a
        // column's is as wide as the strip, and as tall when it shows an icon, so a shelf of
        // icons is a column of squares. The button is then drawn at that size, the same way a
        // button anywhere else is
        const glm::vec2 box = row
            ? glm::vec2(extent(*button) + styles_.base().padding, size.y)
            : glm::vec2(size.x, button->icon().empty() ? styles_.base().lineHeight : size.x);
        place(*button, {.position = pen, .size = box});
        pen += row ? glm::vec2(box.x, 0.0F) : glm::vec2(0.0F, box.y);
    }
}

/**
 **/
float Arranger::row(const component::menu::Item* item) const {
    const float line = styles_.base().lineHeight;
    return item != nullptr && item->separator() ? line * separatorShare : line;
}

void Arranger::panel(component::menu::Menu& menu, const glm::vec2& origin, const glm::vec2& room, float shortcuts) const {
    const std::size_t count = menu.count();
    if (count == 0) {
        return;
    }
    float widest = 0.0F;
    float rows = 0.0F;
    for (std::size_t index = 0; index < count; index++) {
        const boost::shared_ptr<component::menu::Item>& item = menu[index];
        widest = std::max(widest, item ? type_.width(item->text()) : 0.0F);
        rows += row(item.get());
    }
    const paint::Dressing& base = styles_.base();
    // a column either side of the labels: the mark on the left and the submenu arrow on the
    // right. Both are there whether or not this menu uses them, so that every label in one
    // panel starts at the same place. Shortcuts take a column of their own, a padding clear
    // of the labels
    const float column = base.lineHeight * markColumn;
    const float keys = shortcuts > 0.0F ? shortcuts + base.padding : 0.0F;
    const glm::vec2 size(widest + keys + (column * 2.0F) + (base.padding * 0.5F), rows + (base.panelPadding * 2.0F));

    // a panel that would hang off an edge is moved back onto the canvas rather than clipped,
    // so the flyouts of a bar's last menu stay inside the window
    glm::vec2 corner(std::min(origin.x, room.x - size.x), std::min(origin.y, room.y - size.y));
    corner = glm::vec2(std::max(corner.x, 0.0F), std::max(corner.y, 0.0F));
    place(menu, {.position = corner, .size = size});

    float top = corner.y + base.panelPadding;
    for (std::size_t index = 0; index < count; index++) {
        const boost::shared_ptr<component::menu::Item>& item = menu[index];
        const float height = row(item.get());
        if (item) {
            place(*item, {.position = glm::vec2(corner.x, top), .size = glm::vec2(size.x, height)});
        }
        top += height;
    }
}

/**
 **/
void Arranger::emphasis(const paint::Emphasis& sets) {
    emphasis_ = sets;
}

/**
 **/
v3d::type::geometry::Bound2D Arranger::safe(const glm::vec2& room) noexcept {
    const glm::vec2 inset(room.x * safeSides, room.y * safeEnds);
    return v3d::type::geometry::Bound2D({.position = inset, .size = room - (inset * 2.0F)});
}

/**
 **/
float Arranger::heading(const component::menu::Menu& level) const {
    if (level.title().empty()) {
        return 0.0F;
    }
    const paint::Dressing& base = styles_.base();
    const float scale = emphasis_.title.type.set() ? emphasis_.title.scale : 1.0F;
    return (base.lineHeight * scale) + (base.padding * 0.5F);
}

/**
 **/
float Arranger::choice(const component::menu::Item* item) const {
    if (item == nullptr) {
        return 0.0F;
    }
    const std::string_view text = item->label();
    const float body = type_.width(text);
    return emphasis_.chosen.type.set() ? std::max(body, emphasis_.chosen.type.width(text)) : body;
}

/**
 **/
void Arranger::centred(component::menu::Menu& level, const glm::vec2& room, float values) const {
    const std::size_t count = level.count();
    if (count == 0) {
        return;
    }
    float widest = 0.0F;
    float rows = 0.0F;
    for (std::size_t index = 0; index < count; index++) {
        const boost::shared_ptr<component::menu::Item>& item = level[index];
        widest = std::max(widest, choice(item.get()));
        rows += row(item.get());
    }
    const paint::Dressing& base = styles_.base();
    // a value sits two paddings clear of the longest label, so a row reads as two columns
    if (values > 0.0F) {
        widest += (base.padding * 2.0F) + values;
    }
    if (!level.title().empty()) {
        const paint::Type& titled = emphasis_.title.type.set() ? emphasis_.title.type : type_;
        widest = std::max(widest, titled.width(level.title()));
    }
    const float band = heading(level);
    const glm::vec2 size(widest + (base.padding * 2.0F), band + rows + (base.padding * 2.0F));
    const v3d::type::geometry::Bound2D area = safe(room);
    const glm::vec2 corner = glm::max(area.position() + ((area.size() - size) * 0.5F), area.position());
    place(level, {.position = corner, .size = size});

    float top = corner.y + base.padding + band;
    for (std::size_t index = 0; index < count; index++) {
        const boost::shared_ptr<component::menu::Item>& item = level[index];
        const float height = row(item.get());
        if (item) {
            place(*item, {.position = glm::vec2(corner.x, top), .size = glm::vec2(size.x, height)});
        }
        top += height;
    }
}

/**
 **/
v3d::type::geometry::Bound2D Arranger::lineRoom(bool vertical, const v3d::type::geometry::Bound2D& bounds) {
    // the children share the room along the line, so none of them is offered any of it. An
    // Auto extent along the line is the child's natural size, and a child with no natural
    // size takes nothing. Across the line each child is offered the whole room, and
    // stretch() makes each child fill it
    const glm::vec2 extent = bounds.size();
    return v3d::type::geometry::Bound2D({.position = bounds.position(), .size = vertical ? glm::vec2(extent.x, 0.0F) : glm::vec2(0.0F, extent.y)});
}

/**
 **/
glm::vec2 Arranger::childSize(Component& child, const v3d::type::geometry::Bound2D& room, const glm::vec2& extent) const {
    const glm::vec2 own = natural(child, room);
    const Layout& layout = child.layout();
    return glm::vec2(layout.width.resolve({.extent = extent.x, .own = own.x}), layout.height.resolve({.extent = extent.y, .own = own.y}));
}

std::uint64_t Arranger::nextSerial() noexcept {
    static std::atomic<std::uint64_t> next{1};  // process-wide: a counter of serials unique across the run
    return next++;
}

};  // namespace v3d::ui
