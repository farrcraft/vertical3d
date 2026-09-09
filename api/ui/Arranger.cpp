/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Arranger.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "Component.h"
#include "Container.h"
#include "component/Box.h"
#include "component/Button.h"
#include "component/CheckBox.h"
#include "component/Icon.h"
#include "component/Label.h"
#include "component/Scrollbar.h"
#include "component/SelectList.h"
#include "component/TabBar.h"
#include "component/TabPage.h"
#include "component/TextBox.h"
#include "component/Toolbar.h"
#include "component/Type.h"
#include "component/menu/MenuBar.h"
#include "style/Resolver.h"

#include "../render/realtime/Canvas.h"

namespace v3d::ui {

const float Arranger::ruleWidth = 1.0f;

namespace {

/**
 * Leave a component holding the bounds it was put in, which is what the cursor is tested
 * against per ADR-0019.
 *
 * Through a reference to the base, because a menu's own size() is its item count and hides
 * the one that means how big it is.
 **/
void place(Component& component, const glm::vec2& position, const glm::vec2& size) {
    component.position(position);
    component.size(size);
}

};  // namespace

Arranger::Arranger(const Measure& measure, const style::Resolver& styles) :
    measure_(measure),
    styles_(styles) {
}

/**
 **/
void Arranger::walk(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<Component>& component,
    const v3d::type::Bound2D& bounds, const Paint& paint) const {
    if (!component || !component->visible()) {
        return;
    }
    place(*component, bounds.position(), bounds.size());

    if (paint) {
        paint(canvas, component);
    }

    // only the chosen page of a tab bar is walked, so a page that is not up has no box and
    // nothing in it can be picked
    if (component->type() == component::Type::TabBar) {
        const auto* tabs = static_cast<const component::TabBar*>(component.get());
        walk(canvas, tabs->page(), page(*tabs), paint);
        return;
    }

    const std::vector<boost::shared_ptr<Component>>& children = component->children();
    if (children.empty()) {
        return;
    }

    // a component that holds more than it can show cuts what it holds off at its own box,
    // per ADR-0037. It is what the component asked for rather than the default, because a
    // menu drops a panel out of the strip it came from
    // a clip is the canvas's, so a walk asked for boxes alone has nothing to push it onto
    const bool cut = component->clip() && canvas != nullptr;
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
            walk(canvas, children[index], boxes[index], paint);
        }
    } else {
        std::vector<boost::shared_ptr<Component>> sorted;
        if (!inDrawOrder(children)) {
            sorted = v3d::ui::ordered(children);
        }
        const v3d::type::Bound2D room = component->bound();
        for (const boost::shared_ptr<Component>& child : sorted.empty() ? children : sorted) {
            walk(canvas, child, child->layout().resolve(room, natural(*child, room)), paint);
        }
    }

    if (cut) {
        canvas->unclip();
    }
}

/**
 **/
glm::vec2 Arranger::natural(Component& component, const v3d::type::Bound2D& room) const {
    switch (component.type()) {
        case component::Type::Label: {
            const auto* label = dynamic_cast<const component::Label*>(&component);
            if (label == nullptr) {
                return glm::vec2(0.0f, 0.0f);
            }
            const float line = measure_(label->text());
            // a label given a width wraps to it, and what it makes of the other axis is the
            // rows it came to - which is what an Auto height is offered, per ADR-0039
            if (component.layout().width.unit() == Length::Unit::Auto) {
                return glm::vec2(line, styles_.base().lineHeight);
            }
            const float width = component.layout().width.resolve(room.size().x, line);
            const std::size_t rows = wrap(label->text(), width, measure_).size();
            return glm::vec2(line,
                styles_.base().lineHeight * static_cast<float>(std::max<std::size_t>(rows, 1)));
        }
        case component::Type::Icon:
            // an icon given no size is a square the height of a strip, which is the one
            // size the ui has that is not derived from a string
            return glm::vec2(styles_.base().barHeight, styles_.base().barHeight);
        case component::Type::Button: {
            const auto* button = dynamic_cast<const component::Button*>(&component);
            return button == nullptr ? glm::vec2(0.0f, 0.0f)
                : glm::vec2(extent(*button) + styles_.base().padding, styles_.base().barHeight);
        }
        case component::Type::CheckBox:
        case component::Type::RadioButton: {
            // the mark, the gap after it and the label, on a row as tall as the taller of
            // the mark and a line of text
            const auto* box = dynamic_cast<const component::CheckBox*>(&component);
            if (box == nullptr) {
                return glm::vec2(0.0f, 0.0f);
            }
            // its own style class, because the mark it is asking room for is drawn at the
            // size that class names - laying out against the base would size the row for a
            // mark of a different size than the one drawn in it
            const Dressing& dress = styles_.resolve(
                component.type() == component::Type::RadioButton
                    ? style::Resolver::Class::Radio : style::Resolver::Class::CheckBox,
                component.style());
            const float text = box->label().empty() ? 0.0f
                : dress.padding * 0.5f + measure_(box->label());
            return glm::vec2(dress.markSize + text, std::max(dress.markSize, dress.lineHeight));
        }
        case component::Type::SelectList: {
            // a list decides how wide its widest row is and nothing about how tall it is:
            // how many rows it shows is what it was given room for
            auto* list = dynamic_cast<component::SelectList*>(&component);
            if (list == nullptr) {
                return glm::vec2(0.0f, 0.0f);
            }
            // measuring every row is what this costs, and the answer only changes when the
            // rows do - so the list keeps it and forgets it when it is given new ones
            float widest = list->widest();
            if (widest < 0.0f) {
                widest = 0.0f;
                for (const std::string& item : list->items()) {
                    widest = std::max(widest, measure_(item));
                }
                list->widest(widest);
            }
            return glm::vec2(
                widest + styles_.resolve(style::Resolver::Class::List, component.style()).padding,
                room.size().y);
        }
        case component::Type::TextBox: {
            // a box is as wide as the room it is in and as tall as the line it holds: the
            // text it will be typed into is not what should size it, or it would grow
            // under the caret
            const Dressing& dress = styles_.resolve(style::Resolver::Class::TextBox, component.style());
            return glm::vec2(room.size().x, dress.lineHeight + dress.padding);
        }
        case component::Type::Scrollbar: {
            // a scrollbar decides how thick it is and nothing about how long: its length
            // is the box it runs down, which is its parent's rather than its own
            const auto* bar = dynamic_cast<const component::Scrollbar*>(&component);
            if (bar == nullptr) {
                return glm::vec2(0.0f, 0.0f);
            }
            return bar->direction() == component::Scrollbar::Direction::Vertical
                ? glm::vec2(styles_.base().scrollbarWidth, room.size().y)
                : glm::vec2(room.size().x, styles_.base().scrollbarWidth);
        }
        case component::Type::Bar:
        case component::Type::HorizontalBox:
        case component::Type::Menu:
        case component::Type::MenuBar:
        case component::Type::MenuItem:
        case component::Type::Panel:
        case component::Type::TabBar:
        case component::Type::TabPage:
        case component::Type::Toolbar:
        case component::Type::Undefined:
        case component::Type::VerticalBox:
            // a panel, a bar and a box decide nothing for themselves, so an Auto extent on
            // one is the room it is in
            break;
    }
    // every enumerator is handled above and the switch carries no default, so C4062 names
    // this function when a component type is added - see ADR-0047
    return room.size();
}

/**
 **/
void Arranger::arrange(const component::Box& box, const v3d::type::Bound2D& bounds,
    std::vector<v3d::type::Bound2D>* boxes) const {
    const bool vertical = box.type() == component::Type::VerticalBox;
    const glm::vec2 extent = bounds.size();
    float pen = vertical ? bounds.position().y : bounds.position().x;

    // along the line the children share the room, so none of them is offered any of it: an
    // Auto extent there is what the child makes of itself, and a child that makes nothing of
    // itself asks for nothing. Across the line each is offered the whole of it, which is what
    // stretch() then insists on
    const v3d::type::Bound2D room(bounds.position(),
        vertical ? glm::vec2(extent.x, 0.0f) : glm::vec2(0.0f, extent.y));

    for (const boost::shared_ptr<Component>& child : box.children()) {
        if (!child || !child->visible()) {
            // a hidden row leaves no gap behind it, which is what makes a list of however
            // many rows there are read as one
            boxes->push_back(v3d::type::Bound2D(bounds.position(), glm::vec2(0.0f, 0.0f)));
            continue;
        }
        const glm::vec2 own = natural(*child, room);
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



glm::vec2 Arranger::stack(const Container& container,
    std::vector<std::pair<boost::shared_ptr<component::Toolbar>, glm::vec2>>* strips,
    std::vector<boost::shared_ptr<component::MenuBar>>* bars) const {
    // what the strips before this one have taken off the top and the left edges, which is
    // where the next one starts
    glm::vec2 taken(0.0f, 0.0f);
    for (const boost::shared_ptr<Component>& component : container.ordered()) {
        if (!component || !component->visible()) {
            continue;
        }
        if (component->type() == component::Type::MenuBar) {
            if (bars != nullptr) {
                bars->push_back(boost::dynamic_pointer_cast<component::MenuBar>(component));
            }
            taken.y += styles_.base().barHeight + ruleWidth;
            continue;
        }
        if (component->type() != component::Type::Toolbar) {
            continue;
        }
        const boost::shared_ptr<component::Toolbar> bar =
            boost::dynamic_pointer_cast<component::Toolbar>(component);
        if (!bar) {
            continue;
        }
        const glm::vec2 corner = bar->edge() == component::Toolbar::Edge::Top
            ? glm::vec2(0.0f, taken.y) : taken;
        if (strips != nullptr) {
            strips->push_back(std::make_pair(bar, corner));
        }
        if (bar->edge() == component::Toolbar::Edge::Top) {
            taken.y += styles_.base().barHeight + ruleWidth;
        } else {
            // what the strip will be drawn at rather than the box it was last drawn in,
            // which is nothing until it has been drawn once
            taken.x += widest(*bar) + styles_.base().padding + ruleWidth;
        }
    }
    return taken;
}

/**
 **/
v3d::type::Bound2D Arranger::page(const component::TabBar& bar) const {
    const float height = styles_.resolve(style::Resolver::Class::Tabs, bar.style()).barHeight;
    const glm::vec2 min = bar.position();
    const glm::vec2 size = bar.size();
    return v3d::type::Bound2D(glm::vec2(min.x, min.y + height + ruleWidth),
        glm::vec2(size.x, std::max(size.y - height - ruleWidth, 0.0f)));
}

/**
 **/
float Arranger::extent(const component::Button& button) const {
    // what the button asks a strip for, which is the icon it names rather than the
    // texture it holds - a strip is laid out before anything has been resolved
    if (!button.icon().empty()) {
        return styles_.base().iconSize;
    }
    return measure_(button.label());
}

/**
 **/
float Arranger::widest(const component::Toolbar& bar) const {
    float widest = 0.0f;
    for (std::size_t index = 0; index < bar.size(); index++) {
        const boost::shared_ptr<component::Button> button = bar.button(index);
        if (button) {
            widest = std::max(widest, extent(*button));
        }
    }
    return widest;
}

};  // namespace v3d::ui
