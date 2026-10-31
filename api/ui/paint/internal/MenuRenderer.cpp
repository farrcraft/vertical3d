/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "MenuRenderer.h"

#include <api/event/Event.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/type/geometry/Bound2D.h>
#include <api/ui/Arranger.h>
#include <api/ui/Component.h>
#include <api/ui/Image.h>
#include <api/ui/Transition.h>
#include <api/ui/component/menu/Bar.h>
#include <api/ui/component/menu/Item.h>
#include <api/ui/component/menu/Menu.h>
#include <api/ui/component/menu/Type.h>
#include <api/ui/paint/Dressing.h>
#include <api/ui/paint/Painter.h>
#include <api/ui/paint/Text.h>
#include <api/ui/style/Resolver.h>

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include "Easing.h"

namespace v3d::ui::paint::internal {

MenuRenderer::MenuRenderer(const Measure& measure, const Write& write, const style::Resolver& styles,
    const Arranger& arranger, const Easing& easing) :
    measure_(measure),
    write_(write),
    styles_(styles),
    arranger_(arranger),
    easing_(easing) {
}

const Dressing& MenuRenderer::base() const noexcept {
    return styles_.base();
}

void MenuRenderer::shortcuts(const Shortcut& lookup) {
    shortcut_ = lookup;
}

std::string MenuRenderer::shortcut(const v3d::event::Event& command) const {
    if (!shortcut_ || !command.context()) {
        return std::string();
    }
    return shortcut_(command);
}

void MenuRenderer::begin(v3d::render::realtime::canvas::Canvas* canvas, float open) const {
    canvas->fade(open);
    canvas->push();
    canvas->translate(glm::vec2(0.0F, -(1.0F - open) * base().openDistance));
}

void MenuRenderer::end(v3d::render::realtime::canvas::Canvas* canvas) {
    canvas->pop();
    canvas->unfade();
}

/**
 **/
void MenuRenderer::draw(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::menu::Menu>& menu) const {
    if (canvas == nullptr || !menu) {
        return;
    }

    boost::shared_ptr<component::menu::Menu> level = menu->level();
    if (!level) {
        level = menu;
    }
    const std::size_t count = level->count();
    if (count == 0) {
        return;
    }

    arranger_.centred(*level, glm::vec2(static_cast<float>(canvas->width()), static_cast<float>(canvas->height())));
    const glm::vec2 origin = level->position();
    const float width = static_cast<const Component&>(*level).size().x;
    const float height = static_cast<const Component&>(*level).size().y;

    // the border is traced just outside the box the items are laid out in
    const glm::vec2 rim(base().borderWidth, base().borderWidth);
    begin(canvas, easing_.opened(&level->motion().opening));
    cast(canvas, origin - rim, origin + glm::vec2(width, height) + rim);
    plateBox(canvas, origin - rim, origin + glm::vec2(width, height) + rim, base().radius, base().borderWidth,
        {.inside = base().panel, .outline = base().border});

    const boost::shared_ptr<component::menu::Item> active = level->active();

    for (std::size_t index = 0; index < count; index++) {
        const boost::shared_ptr<component::menu::Item>& item = (*level)[index];
        if (!item) {
            continue;
        }
        const float top = item->position().y;
        if (item->separator()) {
            rule(canvas, origin.x + base().padding, origin.x + width - base().padding, top + (item->size().y * 0.5F));
            continue;
        }
        const bool selected = active && item == active;

        const glm::vec4 lit = selected ? easing_.toward(&item->motion().face, base().highlight) : easing_.away(&item->motion().face);
        if (lit.a > 0.0F) {
            fillBox(canvas,
                glm::vec2(origin.x + (base().padding * 0.5F), top),
                glm::vec2(origin.x + width - (base().padding * 0.5F), top + base().lineHeight),
                base().radius, lit);
        }

        // the pen sits on the baseline, most of the way down the line box. Descenders go in
        // the space below it
        const glm::vec2 pen(origin.x + base().padding, top + (base().lineHeight * LINE_BASELINE));
        write_(item->text(), pen, easing_.toward(&item->motion().ink, itemInk(item, base(), selected)));
    }
    end(canvas);
}

/**
 **/
void MenuRenderer::draw(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::menu::Bar>& bar) const {
    if (canvas == nullptr || !bar) {
        return;
    }
    // a submenu waits for the cursor to rest on its item only when there is a clock to wait on
    if (easing_.timed()) {
        bar->delay(base().submenuDelay);
        bar->settle(easing_.clock().now);
    }

    // a docked bar runs along the top of the canvas, and any other is where its layout puts it
    const glm::vec2 room(static_cast<float>(canvas->width()), static_cast<float>(canvas->height()));
    const v3d::type::geometry::Bound2D box = bar->docked() ?
        v3d::type::geometry::Bound2D({.position = glm::vec2(0.0F, 0.0F), .size = glm::vec2(room.x, base().barHeight)}) :
        bar->layout().resolve(v3d::type::geometry::Bound2D({.position = glm::vec2(0.0F, 0.0F), .size = room}),
            glm::vec2(room.x, base().barHeight));
    const glm::vec2 corner = box.position();
    const float width = box.size().x;
    const float height = box.size().y;
    Arranger::place(*bar, {.position = corner, .size = box.size()});

    canvas->rect(corner, corner + glm::vec2(width, height), base().panel);
    // a rule along the bottom edge, so the strip reads as something over the scene rather
    // than as part of it
    canvas->rect(corner + glm::vec2(0.0F, height), corner + glm::vec2(width, height + Arranger::ruleWidth), base().border);

    float pen = corner.x + (base().padding * 0.5F);
    for (std::size_t index = 0; index < bar->count(); index++) {
        const boost::shared_ptr<component::menu::Menu> menu = bar->menu(index);
        if (!menu) {
            continue;
        }
        const std::string& label = bar->label(index);
        const float extent = measure_(label) + base().padding;

        // on the bar rather than on the menu, whose own bounds are the panel it drops
        bar->place(index, glm::vec2(pen, corner.y), glm::vec2(extent, height));

        const bool lit = bar->open() == static_cast<int>(index) || bar->hover() == static_cast<int>(index);
        Motion& motion = menu->motion();
        const glm::vec4 face = lit ? easing_.toward(&motion.face, base().highlight) : easing_.away(&motion.face);
        if (face.a > 0.0F) {
            canvas->rect(glm::vec2(pen, corner.y), glm::vec2(pen + extent, corner.y + height), face);
        }
        write_(label, glm::vec2(pen + (base().padding * 0.5F), corner.y + (height * WIDGET_BASELINE)),
            easing_.toward(&motion.ink, lit ? base().activeText : base().text));
        pen += extent;
    }

    // outermost first, so that a flyout is drawn over the panel it came out of
    const std::vector<boost::shared_ptr<component::menu::Menu>>& panels = bar->panels();
    for (std::size_t depth = 0; depth < panels.size(); depth++) {
        glm::vec2 origin;
        if (depth == 0) {
            origin = glm::vec2(bar->bound(static_cast<std::size_t>(bar->open())).position().x, corner.y + height);
        } else {
            // out of the right hand edge of the parent, level with the item it came from
            const v3d::type::geometry::Bound2D bounds = panels[depth - 1]->bound();
            const boost::shared_ptr<component::menu::Item> item = panels[depth - 1]->active();
            origin = glm::vec2(bounds.position().x + bounds.size().x,
                item ? item->position().y : bounds.position().y);
        }
        panel(canvas, panels[depth], origin);
    }
}

/**
 **/
void MenuRenderer::cast(v3d::render::realtime::canvas::Canvas* canvas, const glm::vec2& min, const glm::vec2& max) const {
    const glm::vec2 drop(0.0F, base().shadowOffset);
    shadowBox(canvas, min + drop, max + drop, {.radius = base().radius, .reach = base().shadowSize, .colour = base().shadow});
}

/**
 **/
void MenuRenderer::panel(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::menu::Menu>& menu,
    const glm::vec2& origin) const {
    const std::size_t count = menu->count();
    if (count == 0) {
        return;
    }

    // each item's shortcut, worked out once for both the panel's width and its drawing
    std::vector<std::string> keys(count);
    float widest = 0.0F;
    for (std::size_t index = 0; index < count; index++) {
        const boost::shared_ptr<component::menu::Item>& item = (*menu)[index];
        if (item && item->itemType() != component::menu::ItemType::Submenu && !item->separator()) {
            keys[index] = shortcut(item->event());
            widest = std::max(widest, measure_(keys[index]));
        }
    }

    arranger_.panel(*menu, origin, glm::vec2(static_cast<float>(canvas->width()), static_cast<float>(canvas->height())), widest);
    const glm::vec2 corner = menu->position();
    const glm::vec2 size = static_cast<const Component&>(*menu).size();
    const float column = base().lineHeight * Arranger::markColumn;

    // the border is traced just outside the box the items are laid out in
    const glm::vec2 rim(base().borderWidth, base().borderWidth);
    begin(canvas, easing_.opened(&menu->motion().opening));
    cast(canvas, corner - rim, corner + size + rim);
    plateBox(canvas, corner - rim, corner + size + rim, base().radius, base().borderWidth,
        {.inside = base().panel, .outline = base().border});

    const boost::shared_ptr<component::menu::Item> active = menu->active();

    for (std::size_t index = 0; index < count; index++) {
        const boost::shared_ptr<component::menu::Item>& item = (*menu)[index];
        if (!item) {
            continue;
        }
        if (item->separator()) {
            rule(canvas, corner.x + (column * 0.5F), corner.x + size.x - (column * 0.5F), item->position().y + (item->size().y * 0.5F));
            continue;
        }
        entry(canvas, item, menu->bound(), active && item == active, keys[index]);
    }
    end(canvas);
}

/**
 **/
void MenuRenderer::entry(v3d::render::realtime::canvas::Canvas* canvas, const boost::shared_ptr<component::menu::Item>& item,
    const v3d::type::geometry::Bound2D& panel, bool selected, const std::string& keys) const {
    const glm::vec2 corner = panel.position();
    const glm::vec2 size = panel.size();
    const float column = base().lineHeight * Arranger::markColumn;
    const float top = item->position().y;

    const glm::vec4 lit = selected ? easing_.toward(&item->motion().face, base().highlight) : easing_.away(&item->motion().face);
    if (lit.a > 0.0F) {
        canvas->rect(glm::vec2(corner.x, top), glm::vec2(corner.x + size.x, top + base().lineHeight), lit);
    }

    const glm::vec2 centre(corner.x + (column * 0.5F), top + (base().lineHeight * 0.5F));
    const v3d::ui::Image& picture = item->image();
    if (item->checked()) {
        const float mark = base().lineHeight * checkMarkScale;
        canvas->rect(centre - glm::vec2(mark, mark), centre + glm::vec2(mark, mark), base().activeText);
    } else if (picture.valid()) {
        // a check mark takes the column before an icon does, so a checked item shows its state.
        // An item that cannot be used greys its icon as it greys its label
        const float half = std::min(base().iconSize, column) * 0.5F;
        canvas->rect(centre - glm::vec2(half, half), centre + glm::vec2(half, half), picture.uv0, picture.uv1,
            usable(*item) ? glm::vec4(1.0F, 1.0F, 1.0F, 1.0F) : base().disabledText, picture.texture);
    }

    if (item->itemType() == component::menu::ItemType::Submenu) {
        // a three sided circle is a triangle with a vertex at zero degrees, which
        // points along +x
        canvas->circle(glm::vec2(corner.x + size.x - (column * 0.5F), top + (base().lineHeight * 0.5F)),
            base().lineHeight * submenuArrowScale, 3, selected ? base().activeText : base().text);
    }

    const float baseline = top + (base().lineHeight * LINE_BASELINE);
    write_(item->text(), glm::vec2(corner.x + column, baseline), easing_.toward(&item->motion().ink, itemInk(item, base(), selected)));
    if (!keys.empty()) {
        // right-aligned against the arrow column, and quieter than the label except on the
        // chosen row, where only active text reads against the highlight
        const glm::vec2 pen(corner.x + size.x - column - measure_(keys), baseline);
        glm::vec4 colour = usable(*item) ? base().secondaryText : base().disabledText;
        if (selected && usable(*item)) {
            colour = base().activeText;
        }
        write_(keys, pen, easing_.toward(&item->motion().shortcut, colour));
    }
}

/**
 **/
void MenuRenderer::rule(v3d::render::realtime::canvas::Canvas* canvas, float left, float right, float middle) const {
    const float half = Arranger::ruleWidth * 0.5F;
    canvas->rect(glm::vec2(left, middle - half), glm::vec2(right, middle + half), base().rule);
}

glm::vec4 MenuRenderer::itemInk(const boost::shared_ptr<component::menu::Item>& item, const Dressing& dress, bool selected) {
    if (!item || !usable(*item)) {
        return dress.disabledText;
    }
    return selected ? dress.activeText : dress.text;
}

};  // namespace v3d::ui::paint::internal
