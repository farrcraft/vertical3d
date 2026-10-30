/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <glm/vec4.hpp>

#include "Dressing.h"

namespace v3d::ui::paint {

/**
 * Two scales of colour worked out from a base colour, an accent colour and a contrast, and
 * the colour of every Dressing role taken from them.
 *
 * The neutral scale has twelve steps of one hue, from the darkest surface to the brightest
 * text. The base colour is its third step, the surface a panel is drawn on. The accent scale
 * has twelve steps of the accent's hue, and the accent colour is its ninth step, the solid
 * accent. Each step's lightness is a fixed distance from the base's or the accent's, in the
 * OKLCH colour space, where equal distances look equally far apart.
 *
 * The contrast multiplies every distance, so above 1 the steps spread apart and below 1 they
 * draw together. A base lighter than mid grey is a light theme: the steps run the other way,
 * and text is darker than its surface.
 *
 * Each role is one step of one scale. The highlight is the exception: it starts one step below
 * the solid accent, and moves away from the active text until the two meet 4.5:1.
 **/
class Palette final {
 public:
    /** How many steps each scale has. **/
    static constexpr std::size_t steps = 12;

    using Scale = std::array<glm::vec4, steps>;

    /**
     * Which ui model the roles are for. The immediate layer's tracks and tabs sit at the level
     * of its widgets, a step above the retained ui's.
     **/
    enum class Use : std::uint8_t {
        Chrome,
        Tools
    };

    /**
     * One role and the colour the palette gives it.
     **/
    struct Role final {
        glm::vec4 Dressing::* field;
        glm::vec4 colour;
    };

    /** How many colour roles a Dressing has, each of which a palette gives a colour. **/
    static constexpr std::size_t roleCount = 26;

    using Roles = std::array<Role, roleCount>;

    /** The base colour a palette is built from when a theme names none, #1d1f23. **/
    static constexpr glm::vec4 base{0.113725F, 0.121569F, 0.137255F, 1.0F};
    /** The accent colour a palette is built from when a theme names none, #4d8ce6. **/
    static constexpr glm::vec4 accent{0.301961F, 0.549020F, 0.901961F, 1.0F};

    /**
     * The palette built from the built-in base and accent, at a contrast of 1.
     **/
    Palette() noexcept;

    /**
     * @param baseColour the surface a panel is drawn on. Its alpha is ignored
     * @param accentColour the solid accent. Its alpha is ignored
     * @param contrast how far apart the steps are, where 1 is the built-in spread. A value
     *      below zero counts as zero
     **/
    Palette(const glm::vec4& baseColour, const glm::vec4& accentColour, float contrast) noexcept;

    const Scale& neutral() const noexcept;
    const Scale& accented() const noexcept;

    /**
     * @return whether text is lighter than the surfaces it is drawn on
     **/
    bool dark() const noexcept;

    /**
     * @return every colour role of a Dressing, with the colour this palette gives it
     **/
    Roles roles(Use use) const noexcept;

    /**
     * Write every colour role into a dressing. Metrics are left as they are.
     **/
    void dress(Use use, Dressing* dressing) const noexcept;

 private:
    Scale neutral_;
    Scale accented_;
    glm::vec4 activeText_;
    glm::vec4 highlight_;
    bool dark_;
};

/**
 * The contrast ratio between two colours, as WCAG defines it: from 1, for two colours of the
 * same luminance, to 21, for black against white. Alpha is ignored.
 **/
float contrastRatio(const glm::vec4& one, const glm::vec4& other) noexcept;

};  // namespace v3d::ui::paint
