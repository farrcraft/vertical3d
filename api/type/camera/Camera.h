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
         *	project() divides by w whatever its sign, so a point behind a perspective
         *	camera comes back mirrored through the centre of the viewport as though it
         *	were in front. This returns nothing for such a point, and for a viewport
         *	with no area. An orthographic camera has nothing behind it.
         *	@param point the point, in world space
         *	@param viewport the region the camera draws into, as x, y, width, height
         *	@return the point in window pixels, with its depth in z, or nothing
         */
        std::optional<glm::vec3> screenPoint(const glm::vec3 & point, const int viewport[4]) const;

        /**
         *	The ray a screen point casts into the scene, in world space.
         *	It starts at the near plane and its direction is a unit vector, so a
         *	distance along it is in world units. An orthographic camera gives a ray
         *	parallel to every other, which falls out of unprojecting the two depths
         *	rather than being a case of its own.
         *	The matrices are the cached ones, so createProjection() and createView()
         *	have to have been called for the state the point was clicked against.
         *	@param point where the click was, in window pixels
         *	@param viewport the region the camera draws into, as x, y, width, height
         */
        geometry::Ray ray(const glm::vec2 & point, const int viewport[4]) const;

        /**
         *	The point on a plane under the cursor: ray() followed by the ray's crossing
         *	of the plane, the question a grid game asks of its ground.
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
        /*
            dolly, truck, pedestal are types of translations with special restrictions
            pan and tilt are types of rotation with restrictions
        */
        /**
         *	Pan the camera.
         *	Move horizontally around a fixed axis (the camera's y axis) - camera 
         *	rotation & look at position changes but eye position doesn't - look left/right
         */
        void pan(float angle);
        /**
         *	Tilt the camera.
         *	Move vertically around a fixed axis (camera's x axis) - look up/down
         */
        void tilt(float angle);
        /**
         *	Dolly the camera.
         *	Move eye forward or backward along direction of view
         *	same as pedestal but use direction vector instead of up vector
         */
        void dolly(float d);
        /**
         *	Truck the camera.
         *	Move eye on axis perpendicular to direction of view and up axis - move 
         *	left/right multiply delta value and right vector to get eye delta - 
         *	right vector must be normalized - add eye delta to current eye position
         */
        void truck(float d);
        /**
         *	Zoom the camera.
         *	Affects the camera lens to zoom in or out (dolly without moving camera)
         */
        void zoom(float z);
        /**
         *	Pedestal the camera.
         *	Move eye on up axis - move up/down
         *	same as truck but use up vector instead of right vector
         */
        void pedestal(float d);

        bool orthographic() const;
        void orthographic(bool ortho);
        /**
        *	The world units a single horizontal pixel of the viewport covers.
        *	A screen space drag is turned into a camera move by multiplying it by this.
        *	The factor is a division by the viewport size, so it is zero until the
        *	profile has been given one.
        */
        float orthoFactorHorizontal() const;
        /**
        *	The world units a single vertical pixel of the viewport covers.
        *	The pixel aspect ratio applies to width only, so the two factors are not
        *	interchangeable.
        */
        float orthoFactorVertical() const;
        /**
        *	Rotate the camera by a given amount.
        *	The rotation tells how to transform the camera into the same space 
        *	defined by its normals. The normals are the up, right, and direction
        *	vectors. The default coordinate system with no rotation is a right-
        *	handed system with right +x, up +y, and direction +z.
        *	@param new_rot the new rotation value.
        */
        void rotate(const glm::quat & new_rot);

 private:
        glm::vec3 lookAt_;
        glm::mat4x4 projection_{1.0f};
        glm::mat4x4 view_{1.0f};  // viewing transformation, identity until createView()
        Profile profile_;
};

};  // namespace v3d::type::camera
