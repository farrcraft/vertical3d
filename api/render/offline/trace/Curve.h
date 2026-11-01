/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/geometry/AABBox.h>
#include <api/type/geometry/Ray.h>

#include <array>
#include <optional>
#include <utility>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Primitive.h"

namespace v3d::render::offline::trace {

/**
 * One cubic Bezier segment of a curve, drawn as a flat ribbon of a width that varies along it,
 * in world space.
 *
 * The ribbon faces whatever ray meets it, so it has its full width from every direction and
 * its normal points back along the ray. A camera ray therefore sees it facing the camera, as RI
 * defines a curve. It is found by halving the segment until each piece is nearly straight, in
 * a frame where the ray runs along +z from the origin, and testing how far each piece passes
 * from the ray.
 *
 * Its parameters are v along the segment, from 0 at its first point to 1 at its last, and u
 * across the ribbon, as RI gives a curve's. s and t are u and v.
 **/
class Curve final : public Primitive {
 public:
    /** The four control points, in order. **/
    using Controls = std::array<glm::vec3, 4>;

    /**
     * @param widths the ribbon's width at the segment's start and at its end
     **/
    Curve(const Controls & controls, const glm::vec2 & widths, const glm::vec3 & colour);

    const Controls & controls() const;
    const glm::vec2 & widths() const;

    /** The point at v along the segment's centre line. **/
    glm::vec3 point(float v) const;

    bool intersect(const v3d::type::geometry::Ray & ray, float from, const Pose & pose,
        Intersection* found) const override;
    void describe(const Intersection & found, Hit* hit) const override;
    bool bound(v3d::type::geometry::AABBox* box) const override;

 private:
    /**
     * A search of one segment, in a frame where the ray runs along +z from the origin.
     **/
    class Search final {
     public:
        /**
         * The ray in the space the segment is stored in: where it starts, and its unit direction.
         **/
        struct Local {
            glm::vec3 origin;
            glm::vec3 along;
        };

        Search(const glm::vec2 & widths, float from, const Pose & pose, const Local & local, const v3d::type::geometry::Ray & ray);

        /**
         * Halve a piece until it is nearly straight, keeping the nearest hit on any piece.
         **/
        void pieces(const Curve::Controls & piece, int depth, float start, float end);

        bool found() const;

        const Intersection & nearest() const;

     private:
        float width(float v) const;

        /**
         * Test a piece taken to be straight: whether the ray passes between its two ends, and
         * within half the ribbon's width of the curve there.
         **/
        void straight(const Curve::Controls & piece, float start, float end);

        /**
         * A distance along the local ray as a distance along the ray the scene cast, which a
         * motion that scales does not keep.
         **/
        std::optional<float> along(float local) const;

        glm::vec2 widths_;
        float from_;
        const Pose & pose_;
        glm::vec3 origin_;
        glm::vec3 along_;
        const v3d::type::geometry::Ray & ray_;
        bool found_ = false;
        Intersection nearest_;
    };

    /**
     * The most times a segment is halved before its pieces are tested as straight lines. A
     * thousand pieces are straight enough for any curve a scene draws.
     **/
    static constexpr int DEEPEST = 10;

    /**
     * How far a piece may bow from straight, as a fraction of the ribbon's width, before it is
     * halved again.
     **/
    static constexpr float BOW = 0.05F;

    /**
     * A cubic's degree, which its derivative is this many times the steps between its points.
     **/
    static constexpr float DEGREE = 3.0F;

    static glm::vec3 pointOn(const Controls & controls, float v);

    static glm::vec3 tangentOn(const Controls & controls, float v);

    /**
     * The two halves of a segment, by de Casteljau's construction at its middle.
     **/
    static std::pair<Controls, Controls> halves(const Controls & controls);

    Controls controls_;
    glm::vec2 widths_;
};

};  // namespace v3d::render::offline::trace
