/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "ComponentRenderer.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include "component/Type.h"

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

    };  // namespace

    /**
     **/
    ComponentRenderer::Style::Style() noexcept :
        lineHeight(34.0f),
        padding(24.0f),
        barHeight(28.0f),
        panelPadding(4.0f),
        panel(0.05f, 0.06f, 0.09f, 0.92f),
        border(0.35f, 0.38f, 0.45f, 1.0f),
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

    /**
     **/
    ComponentRenderer::Style& ComponentRenderer::style() noexcept {
        return style_;
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
        // what the strips drawn so far have taken off the top and the left edges, which is
        // where the next one starts
        glm::vec2 taken(0.0f, 0.0f);
        std::vector<boost::shared_ptr<component::MenuBar>> bars;
        for (const boost::shared_ptr<Component>& component : container.components()) {
            if (!component || !component->visible()) {
                continue;
            }
            if (component->type() == component::Type::MENU) {
                draw(canvas, boost::dynamic_pointer_cast<component::Menu>(component));
            } else if (component->type() == component::Type::MENUBAR) {
                // held back to the end: an open menu drops a panel over whatever the strips
                // below it occupy, so it has to be drawn after them
                bars.push_back(boost::dynamic_pointer_cast<component::MenuBar>(component));
                taken.y += style_.barHeight + ruleWidth;
            } else if (component->type() == component::Type::TOOLBAR) {
                const boost::shared_ptr<component::Toolbar> bar =
                    boost::dynamic_pointer_cast<component::Toolbar>(component);
                if (!bar) {
                    continue;
                }
                if (bar->edge() == component::Toolbar::Edge::Top) {
                    draw(canvas, bar, glm::vec2(0.0f, taken.y));
                    taken.y += style_.barHeight + ruleWidth;
                } else {
                    draw(canvas, bar, glm::vec2(taken.x, taken.y));
                    taken.x += bar->bound().size().x + ruleWidth;
                }
            }
        }
        for (const boost::shared_ptr<component::MenuBar>& bar : bars) {
            draw(canvas, bar);
        }
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
            if (component->type() == component::Type::MENUBAR) {
                taken.y += style_.barHeight + ruleWidth;
            } else if (component->type() == component::Type::TOOLBAR) {
                const boost::shared_ptr<component::Toolbar> bar =
                    boost::dynamic_pointer_cast<component::Toolbar>(component);
                if (!bar) {
                    continue;
                }
                if (bar->edge() == component::Toolbar::Edge::Top) {
                    taken.y += style_.barHeight + ruleWidth;
                } else {
                    taken.x += widest(*bar) + style_.padding + ruleWidth;
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

        const float width = widest + style_.padding * 2.0f;
        const float height = style_.lineHeight * static_cast<float>(count) + style_.padding * 2.0f;
        const glm::vec2 origin(
            (static_cast<float>(canvas->width()) - width) * 0.5f,
            (static_cast<float>(canvas->height()) - height) * 0.5f);

        // a one pixel border, as a filled rectangle with the panel drawn over it
        canvas->rect(origin - glm::vec2(1.0f, 1.0f), origin + glm::vec2(width + 1.0f, height + 1.0f), style_.border);
        canvas->rect(origin, origin + glm::vec2(width, height), style_.panel);

        const boost::shared_ptr<component::MenuItem> active = level->active();

        for (std::size_t index = 0; index < count; index++) {
            const float top = origin.y + style_.padding + style_.lineHeight * static_cast<float>(index);
            const bool selected = active && (*level)[index] == active;

            if (selected) {
                canvas->rect(
                    glm::vec2(origin.x + style_.padding * 0.5f, top),
                    glm::vec2(origin.x + width - style_.padding * 0.5f, top + style_.lineHeight),
                    style_.highlight);
            }

            // the pen sits on the baseline, which is most of the way down the line box - the
            // remainder is where descenders go
            const glm::vec2 pen(origin.x + style_.padding, top + style_.lineHeight * 0.75f);
            write_(labels[index], pen, selected ? style_.activeText : style_.text);
        }
    }

    /**
     **/
    void ComponentRenderer::draw(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::MenuBar>& bar) const {
        if (canvas == nullptr || !bar) {
            return;
        }

        const float width = static_cast<float>(canvas->width());
        place(*bar, glm::vec2(0.0f, 0.0f), glm::vec2(width, style_.barHeight));

        canvas->rect(glm::vec2(0.0f, 0.0f), glm::vec2(width, style_.barHeight), style_.panel);
        // a rule along the bottom edge, so the strip reads as something over the scene rather
        // than as part of it
        canvas->rect(glm::vec2(0.0f, style_.barHeight), glm::vec2(width, style_.barHeight + ruleWidth), style_.border);

        float pen = style_.padding * 0.5f;
        for (std::size_t index = 0; index < bar->size(); index++) {
            const boost::shared_ptr<component::Menu> menu = bar->menu(index);
            if (!menu) {
                continue;
            }
            const std::string& label = bar->label(index);
            const float extent = measure_(label) + style_.padding;

            // on the bar rather than on the menu, whose own bounds are the panel it drops
            bar->place(index, glm::vec2(pen, 0.0f), glm::vec2(extent, style_.barHeight));

            const bool lit = bar->open() == static_cast<int>(index) || bar->hover() == static_cast<int>(index);
            if (lit) {
                canvas->rect(glm::vec2(pen, 0.0f), glm::vec2(pen + extent, style_.barHeight), style_.highlight);
            }
            write_(label, glm::vec2(pen + style_.padding * 0.5f, style_.barHeight * 0.7f), lit ? style_.activeText : style_.text);
            pen += extent;
        }

        // outermost first, so that a flyout is drawn over the panel it came out of
        const std::vector<boost::shared_ptr<component::Menu>>& panels = bar->panels();
        for (std::size_t depth = 0; depth < panels.size(); depth++) {
            glm::vec2 origin;
            if (depth == 0) {
                origin = glm::vec2(bar->bound(static_cast<std::size_t>(bar->open())).position().x, style_.barHeight);
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
        float extent = 0.0f;
        for (std::size_t index = 0; index < bar.size(); index++) {
            const boost::shared_ptr<component::Button> button = bar.button(index);
            if (button) {
                extent = std::max(extent, measure_(std::string(button->label())));
            }
        }
        return extent;
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
            ? glm::vec2(static_cast<float>(canvas->width()) - corner.x, style_.barHeight)
            : glm::vec2(widest(*bar) + style_.padding, static_cast<float>(canvas->height()) - corner.y);

        place(*bar, corner, size);

        canvas->rect(corner, corner + size, style_.panel);
        if (row) {
            canvas->rect(glm::vec2(corner.x, corner.y + size.y),
                glm::vec2(corner.x + size.x, corner.y + size.y + ruleWidth), style_.border);
        } else {
            canvas->rect(glm::vec2(corner.x + size.x, corner.y),
                glm::vec2(corner.x + size.x + ruleWidth, corner.y + size.y), style_.border);
        }

        glm::vec2 pen = corner;
        for (std::size_t index = 0; index < bar->size(); index++) {
            const boost::shared_ptr<component::Button> button = bar->button(index);
            if (!button) {
                continue;
            }
            const std::string label(button->label());
            const glm::vec2 extent = row
                ? glm::vec2(measure_(label) + style_.padding, size.y)
                : glm::vec2(size.x, style_.lineHeight);

            place(*button, pen, extent);

            // a checked toggle keeps its highlight whether or not the cursor is on it, which
            // is what says which mask and which tool are in force
            const bool lit = button->checked() || button->state() == component::Button::STATE_HOVER;
            if (lit) {
                canvas->rect(pen, pen + extent, button->checked() ? style_.highlight : style_.hover);
            }

            const glm::vec2 baseline(pen.x + (extent.x - measure_(label)) * 0.5f, pen.y + extent.y * 0.7f);
            write_(label, baseline, lit ? style_.activeText : style_.text);

            pen += row ? glm::vec2(extent.x, 0.0f) : glm::vec2(0.0f, extent.y);
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

        const float column = style_.lineHeight * markColumn;

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
        const glm::vec2 size(widest + column * 2.0f + style_.padding * 0.5f,
            style_.lineHeight * static_cast<float>(count) + style_.panelPadding * 2.0f);

        // a panel that would hang off an edge is moved back onto the canvas rather than
        // clipped, which is what puts the last menu of a bar's flyouts back inside the window
        glm::vec2 corner(
            std::min(origin.x, static_cast<float>(canvas->width()) - size.x),
            std::min(origin.y, static_cast<float>(canvas->height()) - size.y));
        corner = glm::vec2(std::max(corner.x, 0.0f), std::max(corner.y, 0.0f));

        place(*menu, corner, size);

        canvas->rect(corner - glm::vec2(1.0f, 1.0f), corner + size + glm::vec2(1.0f, 1.0f), style_.border);
        canvas->rect(corner, corner + size, style_.panel);

        const boost::shared_ptr<component::MenuItem> active = menu->active();

        for (std::size_t index = 0; index < count; index++) {
            const boost::shared_ptr<component::MenuItem>& item = (*menu)[index];
            const float top = corner.y + style_.panelPadding + style_.lineHeight * static_cast<float>(index);
            const bool selected = active && item == active;

            if (item) {
                place(*item, glm::vec2(corner.x, top), glm::vec2(size.x, style_.lineHeight));
            }

            if (selected) {
                canvas->rect(glm::vec2(corner.x, top), glm::vec2(corner.x + size.x, top + style_.lineHeight), style_.highlight);
            }

            if (item && item->checked()) {
                const glm::vec2 centre(corner.x + column * 0.5f, top + style_.lineHeight * 0.5f);
                const float mark = style_.lineHeight * 0.15f;
                canvas->rect(centre - glm::vec2(mark, mark), centre + glm::vec2(mark, mark), style_.activeText);
            }

            if (item && item->type() == menu::ItemType::Submenu) {
                // a three sided circle is a triangle with a vertex at zero degrees, which
                // points along +x
                canvas->circle(glm::vec2(corner.x + size.x - column * 0.5f, top + style_.lineHeight * 0.5f),
                    style_.lineHeight * 0.18f, 3, selected ? style_.activeText : style_.text);
            }

            const glm::vec2 pen(corner.x + column, top + style_.lineHeight * 0.75f);
            write_(labels[index], pen, selected ? style_.activeText : style_.text);
        }
    }

};  // namespace v3d::ui
