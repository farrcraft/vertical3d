/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/font/Paragraph.h>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Text.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::render::realtime::canvas {
class Canvas;
};  // namespace v3d::render::realtime::canvas

namespace v3d::ui::paint {

class TextRenderer;

/**
 * Text set through one text renderer, at one size and in one weight. A part of the ui measures,
 * wraps and draws its labels through one, and each is laid out as a paragraph.
 *
 * A type with no renderer measures nothing and draws nothing.
 **/
class Type final {
 public:
    Type() = default;

    /**
     * @param size in pixels, or 0 for the renderer's own
     **/
    Type(const boost::shared_ptr<TextRenderer>& text, float size, Weight weight = Weight::Regular);

    /**
     * @return whether it has a renderer
     **/
    bool set() const noexcept;

    /**
     * @return how wide a string is in it, in pixels, including any space at its end
     **/
    float width(std::string_view text) const;

    /**
     * @return the rows a string breaks into at a width, as TextRenderer::rows() breaks them
     **/
    std::vector<std::string> rows(std::string_view text, float width) const;

    /**
     * @return a string laid out on one line, for a caret, a hit or a selection in it. A type with
     *         no renderer gives nothing
     **/
    std::optional<font::Paragraph> paragraph(const std::string& text) const;

    /**
     * Draw a string with its pen on the baseline.
     **/
    void draw(v3d::render::realtime::canvas::Canvas* canvas, std::string_view text, const glm::vec2& pen, const glm::vec4& colour) const;

    /**
     * @return the same renderer at the same size in another weight
     **/
    Type in(Weight weight) const;

    /**
     * @return the same renderer in the same weight at a multiple of the size
     **/
    Type scaled(float factor) const;

    const boost::shared_ptr<TextRenderer>& renderer() const noexcept;

    /**
     * @return the size, in pixels, or 0 for the renderer's own
     **/
    float size() const noexcept;

    Weight weight() const noexcept;

 private:
    boost::shared_ptr<TextRenderer> text_;
    float size_ = 0.0F;
    Weight weight_ = Weight::Regular;
};

/**
 * Text set in another weight or at another size than the body text.
 **/
struct Setting final {
    Type type;             /**< what it is set in. Unset sets the text as the body text is set **/
    float scale { 1.0F };  /**< how large it is against the body text **/
};

/**
 * What a centred menu sets apart from the text of its items: its title, and the label of the
 * chosen item. A setting left unset is set as the body text is.
 **/
struct Emphasis final {
    Setting title;
    Setting chosen;
};

};  // namespace v3d::ui::paint
