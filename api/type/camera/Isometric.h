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
 * to one of four azimuths a quarter turn apart. start() sets where the first of them is.
 *
 * This is not a Camera. It holds the point the view is aimed at and which of the four
 * azimuths it is seen from. apply() writes that onto a Camera, and the Camera builds the
 * matrices. Nothing here reads an input device. A caller maps its own keys or gestures onto
 * rotate(), pan() and zoomBy(), in the same way it passes points to ArcBall.
 *
 * The camera stops only at the four azimuths, never between them, so a scene only has to read
 * well from those four views. pan() is measured in the view's own axes, so dragging right
 * moves the scene right from every azimuth.
 **/
class Isometric {
 public:
    /**
     * The number of azimuths the orbit snaps to. They are a right angle apart.
     **/
    static constexpr int AZIMUTHS = 4;

    /**
     * The default angle above the horizon, in radians - 45 degrees.
     *
     * A true isometric projection uses atan(1 / sqrt(2)), about 35.26 degrees. At that
     * angle the three world axes are equally long on screen. Games usually mean 45 degrees
     * by the word. Pass the true value to elevation() for the strict projection.
     **/
    static constexpr float DEFAULT_ELEVATION = 0.78539816339744830961F;

    /**
     * A start() that puts the four azimuths over the ground diagonals, in radians - 45
     * degrees. The eye then looks across the corner of a square on the ground, so a grid of
     * square tiles is drawn as diamonds. The default start of 0 puts them over the world
     * axes, and the same grid is drawn as squares.
     **/
    static constexpr float DIAGONAL_START = 0.78539816339744830961F;

    /**
     * How far the eye sits from the target, in world units.
     *
     * An orthographic projection does not change with distance, so this decides only what
     * the near and far planes cut. It has to be long enough that the eye clears the tallest
     * thing in the scene.
     **/
    static constexpr float DEFAULT_DISTANCE = 30.0F;

    /**
     * The orthographic half height, in world units - half of what the viewport shows
     * vertically. Smaller is closer in.
     **/
    static constexpr float DEFAULT_ZOOM = 10.0F;
    static constexpr float MINIMUM_ZOOM = 2.0F;
    static constexpr float MAXIMUM_ZOOM = 40.0F;

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
     * valid.
     **/
    void azimuth(int index);

    /**
     * The angle of azimuth 0 around the vertical, in radians. Each later azimuth is a quarter
     * turn counterclockwise from it. At 0 the eye at azimuth 0 sits along +X of the target.
     * Pass DIAGONAL_START to see the ground from its diagonals.
     **/
    float start() const;
    void start(float radians);

    /**
     * Move the target across the ground plane, in the view's own axes: x along right() and
     * y along forward(), both in world units.
     **/
    void pan(const glm::vec2& delta);

    /**
     * The ground plane direction that is to the right on screen at the current azimuth.
     *
     * The cross product follows hand(). A pan computed with the other hand moves the scene
     * the wrong way, while everything else looks correct.
     **/
    glm::vec3 right() const;

    /**
     * The basis this orbit uses. It decides which way right() points, and apply() writes
     * it onto the camera's profile.
     *
     * The orbit stores its own copy because pan() and right() receive no camera, and they
     * must still match what is drawn. Set it here, not on the profile.
     **/
    Profile::Hand hand() const;
    void hand(Profile::Hand hand);

    /**
     * The ground plane direction that is away from the eye on screen at the current
     * azimuth. It is the camera's direction of view flattened onto the ground.
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
     * This does not set the clipping distances, the pixel aspect or the viewport size. The
     * owner of the viewport sets those. It does not rebuild the matrices either, so a frame
     * that moves the camera several times can build them once.
     *
     * It does set the profile's Hand, so the camera draws with the basis that right() and
     * pan() use.
     **/
    void apply(Camera* camera) const;

 private:
    /**
     * The angle of the current azimuth around the vertical, measured from +X towards +Z.
     **/
    float angle() const;

    glm::vec3 target_;
    Profile::Hand hand_{Profile::Hand::UpCrossDirection};
    int azimuth_{0};
    float start_{0.0F};
    float zoom_{DEFAULT_ZOOM};
    float elevation_{DEFAULT_ELEVATION};
    float distance_{DEFAULT_DISTANCE};
};

};  // namespace v3d::type::camera
