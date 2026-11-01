/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Skin.h"

#include <api/render/realtime/canvas/Canvas.h>
#include <api/ui/Component.h>
#include <api/ui/Image.h>
#include <api/ui/component/Button.h>
#include <api/ui/style/Button.h>
#include <api/ui/style/Resolver.h>
#include <api/ui/style/Style.h>
#include <api/ui/style/Theme.h>
#include <api/ui/style/property/Image.h>

#include <algorithm>
#include <string>

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::ui::paint::internal {

Skin::Skin(const style::Resolver& styles) :
    styles_(styles) {
}

bool Skin::draw(v3d::render::realtime::canvas::Canvas* canvas, const component::Button& button,
    const glm::vec2& min, const glm::vec2& max) const {
    const boost::shared_ptr<style::Theme> theme = styles_.theme();
    if (!theme) {
        return false;
    }

    // a theme's button styles differ by state as well as by name, so the matching set is
    // searched for the one whose state matches
    const style::Button::State wanted = look(button);
    boost::shared_ptr<style::Style> target;
    for (const boost::shared_ptr<style::Style>& candidate : theme->getStyleSet(std::string(button.style()), "button")) {
        const boost::shared_ptr<style::Button> styled = boost::dynamic_pointer_cast<style::Button>(candidate);
        if (styled && styled->state() == wanted) {
            target = styled;
            break;
        }
    }
    if (!target) {
        return false;
    }

    float corner = defaultCorner;
    style::readMetric(target, "corner", &corner);
    corner = std::min(corner, std::min((max.x - min.x) * 0.5F, (max.y - min.y) * 0.5F));

    const glm::vec4 white(1.0F, 1.0F, 1.0F, 1.0F);
    unsigned int drawn = 0;

    // every one of the nine is optional: a style naming only a centre is a flat skin, and
    // one naming none at all is not a skin, so the button is drawn flat
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
        const v3d::ui::Image piece = image(target, part.name);
        if (!piece.valid()) {
            continue;
        }
        canvas->rect(part.min, part.max, piece.uv0, piece.uv1, white, piece.texture);
        drawn++;
    }

    return drawn > 0;
}

style::Button::State Skin::look(const component::Button& button) noexcept {
    if (!usable(button)) {
        return style::Button::State::Disabled;
    }
    switch (button.state()) {
        case component::Button::STATE_HOVER: return style::Button::State::Hover;
        case component::Button::STATE_PRESS: return style::Button::State::Press;
        case component::Button::STATE_NORMAL: break;
    }
    return style::Button::State::Normal;
}

v3d::ui::Image Skin::image(const boost::shared_ptr<style::Style>& target, const std::string& name) {
    const boost::shared_ptr<style::property::Image> property =
        boost::dynamic_pointer_cast<style::property::Image>(target->property(name, "image"));
    return property ? property->image() : v3d::ui::Image();
}

};  // namespace v3d::ui::paint::internal
