/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/font/glyph/Curves.h>

#include <vector>

#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>

namespace v3d::font::internal {

/**
 * Builds an outline as quadratic curves from a pen's moves, lines, quadratics and cubics. Every
 * point is mapped through a transform as it is given.
 *
 * A contour is closed by close(), or by the next move, with a line back to where it started. A
 * curve that goes nowhere is left out.
 **/
class CurveBuilder final {
 public:
    /**
     * @param transform maps each point given into the space of the curves built
     **/
    explicit CurveBuilder(const glm::mat3& transform);

    void moveTo(const glm::vec2& to);
    void lineTo(const glm::vec2& to);
    void quadraticTo(const glm::vec2& control, const glm::vec2& to);

    /**
     * Add a cubic, split into CUBIC_PIECES quadratics, each with its control point where the
     * cubic's two would put it.
     **/
    void cubicTo(const glm::vec2& first, const glm::vec2& second, const glm::vec2& to);

    void close();

    /**
     * @return the curves built, every contour closed
     **/
    std::vector<glyph::QuadraticCurve> take();

 private:
    /**
     * How many quadratics a cubic is split into.
     **/
    static constexpr int CUBIC_PIECES = 4;

    /**
     * How far past a cubic's control a quadratic's reaches from the same end, as a share of the
     * way from the end to the control.
     **/
    static constexpr float EXTENDED = 1.5F;

    /**
     * Halfway between two points.
     **/
    static constexpr float HALFWAY = 0.5F;

    /**
     * @return a point mapped into the space of the curves
     **/
    glm::vec2 map(const glm::vec2& point) const;

    /**
     * Add a curve from the pen in the space of the curves, and move the pen to its end.
     **/
    void add(const glm::vec2& control, const glm::vec2& to);

    glm::mat3 transform_;
    glm::vec2 pen_{ 0.0F };
    glm::vec2 start_{ 0.0F };
    std::vector<glyph::QuadraticCurve> curves_;
};

};  // namespace v3d::font::internal
