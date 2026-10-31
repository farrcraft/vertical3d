/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/brep/BRep.h>
#include <api/render/realtime/canvas/Line.h>

#include "Manipulator.h"
#include "Screen.h"
#include "Target.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::editor::manipulator {

/**
 * Moves the selection: an arrow per axis, and a box at the centre.
 *
 * An axis handle moves along that axis by the drag's component along it. The centre
 * handle moves in the plane of the screen.
 **/
class Translate final : public Manipulator {
 public:
    Translate();

    // manipulator overrides
    void draw(const Target& target, const Screen& screen,
        v3d::render::realtime::canvas::Line* canvas) const override;

 protected:
    bool measure(const Placement& seat, const Screen& screen, const glm::vec2& from, const glm::vec2& to,
        Motion* motion) const override;

 private:
    /**
     * The arrow head at the tip of a handle, as a fraction of the handle's length.
     **/
    static constexpr float headLength = 0.22F;
    static constexpr float headRadius = 0.07F;
    /**
     * How many segments the head's ring is drawn with, and how many of them are joined
     * back to the tip.
     **/
    static constexpr unsigned int headSides = 12;
    static constexpr unsigned int headSpokes = 4;
    /**
     * The centre handle's box, as a fraction of a handle's length.
     **/
    static constexpr float centreSize = 0.06F;
};

};  // namespace v3d::editor::manipulator
