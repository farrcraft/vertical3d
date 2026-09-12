/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Camera.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::type::camera {

/**
 * An orthographic camera that orbits a point on the ground at a fixed elevation, snapping
 * to one of four azimuths.
 *
 * A camera behaviour rather than a camera: it holds where the view is aimed and from which
 * of the four corners, and apply() writes that onto a Camera, which is what builds the
 * matrices. Nothing here reads an input device - a caller maps its own keys or gestures onto
 * rotate(), pan() and zoomBy(), the same way ArcBall is handed points rather than events.
 *
 * The four azimuths are what makes it a snap camera rather than a free orbit: everything on
 * the board is authored to be legible from four fixed corners, and no orientation between
 * them has to be. pan() is measured in the view's own axes rather than the world's, so
 * dragging right moves the scene right whichever corner the camera is at.
 **/
class Isometric {
 public:
    /**
     * The number of azimuths the orbit snaps to. Four right angles, so the ground plane's
     * axes stay axis aligned on screen at every one of them.
     **/
    static constexpr int AZIMUTHS = 4;

    /**
     * The default angle above the horizon, in radians - 45 degrees.
     *
     * A true isometric projection is atan(1 / sqrt(2)), about 35.26 degrees, which is the
     * angle that makes the three world axes equal on screen. 45 is the angle games usually
     * mean by the word: it is a whole number of degrees, it halves the depth of the scene
     * on screen rather than compressing it by a third, and a wall drawn at it reads as a
     * wall. Pass the true value to elevation() for the strict projection.
     **/
    static constexpr float DEFAULT_ELEVATION = 0.78539816339744830961f;

    /**
     * How far the eye sits from the target, in world units.
     *
     * An orthographic projection does not change with distance, so this decides only what
     * the near and far planes cut. It has to clear the tallest thing between the eye and
     * the target.
     **/
    static constexpr float DEFAULT_DISTANCE = 30.0f;

    /**
     * The orthographic half height, in world units - half of what the viewport shows
     * vertically. Smaller is closer in.
     **/
    static constexpr float DEFAULT_ZOOM = 10.0f;
    static constexpr float MINIMUM_ZOOM = 2.0f;
    static constexpr float MAXIMUM_ZOOM = 40.0f;

    Isometric();

    /**
     * Turn the orbit by whole azimuth steps, positive counterclockwise. Wraps, so any
     * number of steps is valid.
     **/
    void rotate(int steps);

    /**
     * The azimuth the orbit is snapped to, in [0, AZIMUTHS).
     **/
    int azimuth() const;

    /**
     * Snap to an azimuth without going through the steps between. Wraps, so any index is
     * valid, which is what lets a capture reach all four orientations without driving the
     * camera the way a player would.
     **/
    void azimuth(int index);

    /**
     * Move the target across the ground plane, in the view's own axes: x along right() and
     * y along forward(), both in world units.
     **/
    void pan(const glm::vec2& delta);

    /**
     * The ground plane direction that is to the right on screen at the current azimuth.
     *
     * Derived from the camera basis rather than from the azimuth alone, because which way
     * a basis hands is a convention - see ADR-0012 - and a pan built on the other one
     * moves the scene the wrong way with nothing else looking wrong.
     **/
    glm::vec3 right() const;

    /**
     * The ground plane direction that is away from the eye on screen at the current
     * azimuth, which is the camera's direction of view flattened onto the ground.
     **/
    glm::vec3 forward() const;

    /**
     * Where the eye sits: on the orbit sphere around the target, at the elevation and the
     * azimuth.
     **/
    glm::vec3 eye() const;

    glm::vec3 target() const;
    void target(const glm::vec3& target);

    float zoom() const;

    /**
     * Set the orthographic half height, clamped to [MINIMUM_ZOOM, MAXIMUM_ZOOM].
     **/
    void zoom(float halfHeight);

    /**
     * Widen the view by an amount, clamped the same way. Negative closes in.
     **/
    void zoomBy(float delta);

    float elevation() const;
    void elevation(float radians);

    float distance() const;
    void distance(float distance);

    /**
     * Write this placement onto a camera: the eye, the orientation that aims it at the
     * target, the orthographic half height, and orthographic itself.
     *
     * The clipping distances, the pixel aspect and the viewport size stay the caller's -
     * they belong to whoever owns the viewport rather than to the orbit. The matrices are
     * not rebuilt here either, so a frame that moves the camera several times builds them
     * once, which is what ViewPort does.
     *
     * The profile's Hand stays the caller's too, and lookat() here honours whichever one it
     * is - so an application whose geometry was wound for glm::lookAt sets that hand on its
     * camera once and gets this orbit in the basis it draws through.
     **/
    void apply(Camera* camera) const;

 private:
    glm::vec3 target_;
    int azimuth_{0};
    float zoom_{DEFAULT_ZOOM};
    float elevation_{DEFAULT_ELEVATION};
    float distance_{DEFAULT_DISTANCE};
};

};  // namespace v3d::type::camera
