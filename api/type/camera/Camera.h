/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/geometry/Plane.h>
#include <api/type/geometry/Ray.h>

#include <optional>

#include "Profile.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::type::camera {

/**
 *	A 3D viewing camera.
 */
class Camera {
 public:
        /**
         *	The smallest orthographic half height zoom() leaves, in world units. A zoom drag
         *	is scaled by the current half height, so a half height of zero could never grow
         *	again.
         */
        static constexpr float MINIMUM_ZOOM = 0.01F;

        Camera();
        explicit Camera(const Profile & profile);
        virtual ~Camera();

        // get
        glm::mat4x4 projection() const;
        glm::mat4x4 view() const;

        /**
         * Access the underlying camera profile
         */
        Profile & profile();

        /**
         * Access the underlying camera profile read only.
         */
        const Profile & profile() const;

        glm::vec3 unproject(const glm::vec3 & point, const int viewport[4]) const;
        glm::vec3 project(const glm::vec3 & point, const int viewport[4]) const;

        /**
         *	Where a world point lands on the screen, if it lands anywhere.
         *	project() divides by w whatever its sign. A point behind a perspective camera
         *	therefore comes back mirrored through the centre of the viewport, as though it
         *	were in front. This returns nothing for such a point, and nothing for a viewport
         *	with no area. An orthographic projection leaves w at one, so it rejects no point
         *	for being behind the camera.
         *	@param point the point, in world space
         *	@param viewport the region the camera draws into, as x, y, width, height
         *	@return the point in window pixels, with its depth in z, or nothing
         */
        std::optional<glm::vec3> screenPoint(const glm::vec3 & point, const int viewport[4]) const;

        /**
         *	The ray a screen point casts into the scene, in world space.
         *	It starts at the near plane. Ray normalizes its direction, so a distance along
         *	it is in world units. With an orthographic camera, every ray is parallel to the
         *	view direction.
         *	This reads the cached matrices. Call createProjection() and createView() for
         *	the camera state the point was clicked against first.
         *	@param point where the click was, in window pixels
         *	@param viewport the region the camera draws into, as x, y, width, height
         */
        geometry::Ray ray(const glm::vec2 & point, const int viewport[4]) const;

        /**
         *	The point on a plane under the cursor. It casts ray() and intersects the
         *	result with the plane.
         *	@param cursor where the cursor is, in window pixels
         *	@param viewport the region the camera draws into, as x, y, width, height
         *	@param plane the plane to pick on, such as the ground
         *	@return where the cursor's ray crosses the plane, or nothing when it does not
         *	        cross it ahead of the near plane
         */
        std::optional<glm::vec3> pick(const glm::vec2 & cursor, const int viewport[4],
            const geometry::Plane & plane) const;

        /**
         *	Create a projection matrix.
         *	The resulting matrix can be retrieved by calling projection().
         *	If the camera is set to orthographic then an orthographic projection will be created.
         *	Otherwise a perspective projection will be used.
         */
        void createProjection();
        /**
         *	Create a viewing matrix.
         *	The resulting matrix can be retrieved by calling view().
         *	The viewing matrix contains the translation and rotation portion of  the 
         *	camera transformation.
         */
        void createView();

        // transformations
        /**
         *	Pan the camera: turn it about its own y axis, to look left or right.
         *	The eye does not move.
         */
        void pan(float angle);
        /**
         *	Tilt the camera: turn it about its own x axis, to look up or down.
         *	The eye does not move.
         */
        void tilt(float angle);
        /**
         *	Dolly the camera: move the eye forward or back along the direction vector.
         */
        void dolly(float distance);
        /**
         *	Truck the camera: move the eye left or right along the right vector.
         *	The eye moves by delta times the right vector, so delta is in world units only
         *	while the right vector is normalized.
         */
        void truck(float delta);
        /**
         *	Zoom the camera by changing the orthographic half height. The eye does not
         *	move, and a perspective projection is unaffected.
         *	@param z what to add to the orthographic half height, which stays at or above
         *	MINIMUM_ZOOM
         */
        void zoom(float z);
        /**
         *	Pedestal the camera: move the eye up or down along the up vector.
         */
        void pedestal(float delta);

        bool orthographic() const;
        void orthographic(bool ortho);
        /**
        *	The world units a single horizontal pixel of the viewport covers.
        *	Multiply a drag in pixels by this to get a camera move in world units.
        *	The factor divides by the viewport width. It is zero while the profile's size
        *	is zero.
        */
        float orthoFactorHorizontal() const;
        /**
        *	The world units a single vertical pixel of the viewport covers.
        *	Unlike the horizontal factor, this one leaves out the pixel aspect ratio.
        */
        float orthoFactorVertical() const;
        /**
        *	Rotate the camera by composing a rotation onto its current one, in its own
        *	axes, through Profile::turn(). A zero quaternion leaves the camera unchanged.
        *	ArcBall::drag() returns one for a drag too short to have an axis.
        *	@param new_rot the rotation to compose.
        */
        void rotate(const glm::quat & new_rot);

 private:
        glm::vec3 lookAt_;
        glm::mat4x4 projection_{1.0F};
        glm::mat4x4 view_{1.0F};  // viewing transformation, identity until createView()
        Profile profile_;
};

};  // namespace v3d::type::camera
