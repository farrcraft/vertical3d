/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/Image.h>
#include <api/ui/style/Button.h>

#include <string>

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::render::realtime::canvas {
class Canvas;
};  // namespace v3d::render::realtime::canvas

namespace v3d::ui::component {
class Button;
};  // namespace v3d::ui::component

namespace v3d::ui::style {
class Resolver;
class Style;
};  // namespace v3d::ui::style

namespace v3d::ui::paint::internal {

/**
 * Draws a button from the images its theme names for it.
 *
 * A theme's button styles differ by state as well as by name, so the style is chosen by the
 * state the button is in, rather than resolved once like the other classes.
 **/
class Skin final {
 public:
    /**
     * @param styles where the theme comes from. Held by reference, because the renderer owns
     *      it, and a theme change must reach the skin
     **/
    explicit Skin(const style::Resolver& styles);

    /**
     * Draw the nine images a button style names over the button's box - the four corners
     * at their own size, the four edges stretched along it, and the centre over the rest.
     *
     * @return false when the theme names no image for that button in that state, and a
     *      flat button is drawn instead
     **/
    bool draw(v3d::render::realtime::canvas::Canvas* canvas, const component::Button& button,
        const glm::vec2& min, const glm::vec2& max) const;

 private:
    /**
     * Which of a theme's button styles a button is drawn with. A button that cannot be used
     * takes the disabled one, whatever state the cursor last wrote on it.
     **/
    static style::Button::State look(const component::Button& button) noexcept;

    /**
     * @return what a style's image property was resolved to, unset when the style names no
     *      such image or nothing has resolved it
     **/
    static v3d::ui::Image image(const boost::shared_ptr<style::Style>& target, const std::string& name);

    /**
     * How far in from the edge of a skinned button the corner images reach, when the
     * style names no corner of its own. A texture carries no size a handle can be asked
     * for, so this is a number rather than something measured.
     **/
    static constexpr float defaultCorner = 8.0F;

    const style::Resolver& styles_;
};

};  // namespace v3d::ui::paint::internal
