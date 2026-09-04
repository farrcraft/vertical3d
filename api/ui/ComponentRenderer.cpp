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
#include "style/Button.h"
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
         * Read a colour out of a style, leaving what is there when the style does not name it.
         **/
        void colour(const boost::shared_ptr<Style>& target, const std::string& name, glm::vec4* into) {
            boost::shared_ptr<style::prop::Color> property =
                boost::dynamic_pointer_cast<style::prop::Color>(target->property(name, "color"));
            if (property) {
                *into = property->value();
            }
        }

        /**
         * Read a metric out of a style, leaving what is there when the style does not name it.
         **/
        void metric(const boost::shared_ptr<Style>& target, const std::string& name, float* into) {
            boost::shared_ptr<style::prop::Number> property =
                boost::dynamic_pointer_cast<style::prop::Number>(target->property(name, "number"));
            if (property) {
                *into = property->value();
            }
        }

        /**
         * @return the texture a style's image property was resolved to, unset when the style
         *      names no such image or nothing has resolved it
         **/
        v3d::render::realtime::TextureHandle image(const boost::shared_ptr<Style>& target, const std::string& name) {
            boost::shared_ptr<style::prop::Image> property =
                boost::dynamic_pointer_cast<style::prop::Image>(target->property(name, "image"));
            return property ? property->texture() : v3d::render::realtime::TextureHandle();
        }

    };  // namespace

    /**
     **/
    ComponentRenderer::Style::Style() noexcept :
        lineHeight(34.0f),
        padding(24.0f),
        barHeight(28.0f),
        iconSize(22.0f),
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
    void ComponentRenderer::theme(const boost::shared_ptr<style::Theme>& theme) {
        theme_ = theme;
        if (!theme_) {
            return;
        }

        const boost::shared_ptr<v3d::ui::Style> chrome = lookup("ui", std::string_view());
        if (!chrome) {
            return;
        }

        colour(chrome, "panel", &style_.panel);
        colour(chrome, "border", &style_.border);
        colour(chrome, "text", &style_.text);
        colour(chrome, "active-text", &style_.activeText);
        colour(chrome, "highlight", &style_.highlight);
        colour(chrome, "hover", &style_.hover);

        metric(chrome, "line-height", &style_.lineHeight);
        metric(chrome, "padding", &style_.padding);
        metric(chrome, "bar-height", &style_.barHeight);
        metric(chrome, "icon-size", &style_.iconSize);
        metric(chrome, "panel-padding", &style_.panelPadding);
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
            } else if (component->type() == component::Type::BUTTON) {
                draw(canvas, boost::dynamic_pointer_cast<component::Button>(component));
            } else if (component->type() == component::Type::LABEL) {
                draw(canvas, boost::dynamic_pointer_cast<component::Label>(component));
            } else if (component->type() == component::Type::ICON) {
                draw(canvas, boost::dynamic_pointer_cast<component::Icon>(component));
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
        place(*label, label->position(), glm::vec2(measure_(text), style_.lineHeight));

        const glm::vec2 pen(label->position().x, label->position().y + style_.lineHeight * 0.75f);
        write_(text, pen, style_.text);
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
            size = glm::vec2(style_.barHeight, style_.barHeight);
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
            size = glm::vec2(extent(*button) + style_.padding, style_.barHeight);
        }
        const glm::vec2 min = button->position();
        place(*button, min, size);

        // a checked toggle keeps its highlight whether or not the cursor is on it, which is
        // what says which mask and which tool are in force
        const bool lit = button->checked() || button->state() == component::Button::STATE_HOVER;
        if (!skin(canvas, *button, min, min + size) && lit) {
            canvas->rect(min, min + size, button->checked() ? style_.highlight : style_.hover);
        }

        // an icon is what the button says instead of its label, not as well as it. The label
        // stays on the component for whatever measures it before an image has been resolved
        if (button->texture().valid()) {
            const float side = std::min(style_.iconSize, std::min(size.x, size.y));
            const glm::vec2 corner = min + (size - glm::vec2(side, side)) * 0.5f;
            canvas->rect(corner, corner + glm::vec2(side, side),
                glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), button->texture());
            return;
        }

        const glm::vec2 baseline(min.x + (size.x - measure_(label)) * 0.5f, min.y + size.y * 0.7f);
        write_(label, baseline, lit ? style_.activeText : style_.text);
    }

    /**
     **/
    float ComponentRenderer::extent(const component::Button& button) const {
        // what the button asks a strip for, which is the icon it names rather than the
        // texture it holds - a strip is laid out before anything has been resolved
        if (!button.icon().empty()) {
            return style_.iconSize;
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
        metric(target, "corner", &corner);
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
            // the strip decides how big a button in it is - a row's is as wide as its label
            // and a column's is as wide as the strip - and the button is then drawn at the
            // size it was given, the same way a button anywhere else is
            const glm::vec2 box = row
                ? glm::vec2(extent(*button) + style_.padding, size.y)
                : glm::vec2(size.x, style_.lineHeight);

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
