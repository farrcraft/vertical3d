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

    /**
     **/
    ComponentRenderer::Style::Style() noexcept :
        lineHeight(34.0f),
        padding(24.0f),
        panel(0.05f, 0.06f, 0.09f, 0.92f),
        border(0.35f, 0.38f, 0.45f, 1.0f),
        text(0.78f, 0.80f, 0.84f, 1.0f),
        activeText(1.0f, 1.0f, 1.0f, 1.0f),
        highlight(0.16f, 0.34f, 0.58f, 1.0f) {
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
        for (const boost::shared_ptr<Component>& component : container.components()) {
            if (!component || !component->visible()) {
                continue;
            }
            if (component->type() == component::Type::MENU) {
                draw(canvas, boost::dynamic_pointer_cast<component::Menu>(component));
            }
        }
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

};  // namespace v3d::ui
