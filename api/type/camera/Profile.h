/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <cstdint>
#include <string>

#include <glm/glm.hpp>
#include <glm/ext/quaternion_float.hpp>

namespace v3d::type::camera {
/**
 * A class containing the common camera settings.
 */
class Profile {
 public:
        /**
        *	Which way round lookat() crosses its normals. This decides which world direction
        *	ends up on the right of the screen. The two bases mirror each other
        *	horizontally, and both render. A front face therefore has opposite windings
        *	under the two.
        *
        *	UpCrossDirection is the default. DirectionCrossUp is for an application whose
        *	geometry was authored against glm::lookAt, so that it can use this camera
        *	without changing its renderer.
        */
        enum class Hand : std::uint8_t {
                UpCrossDirection,       /**< right = up x direction **/
                DirectionCrossUp        /**< right = direction x up, which glm::lookAt builds **/
        };

        /** The projection a new profile starts with. **/
        static constexpr float DEFAULT_ORTHO_ZOOM = 1.0F;
        static constexpr float DEFAULT_PIXEL_ASPECT = 1.33F;
        static constexpr float DEFAULT_NEAR = 0.001F;
        static constexpr float DEFAULT_FAR = 100.0F;
        static constexpr float DEFAULT_FOV = 60.0F;

        explicit Profile(const std::string & name);
        /**
        *	Where the eye is, and its up, right and view directions.
        */
        struct Frame {
            glm::vec3 eye{0.0F};
            glm::vec3 up{0.0F, 1.0F, 0.0F};
            glm::vec3 right{1.0F, 0.0F, 0.0F};
            glm::vec3 direction{0.0F, 0.0F, -1.0F};
        };

        /**
        *	Construct a profile with an explicit basis.
        *	The basis vectors are taken as given and the rotation is left at identity.
        *	The view follows the rotation, so orient the profile with lookat(), rotation()
        *	or turn() before drawing.
        *	@param name the profile name.
        *	@param frame the eye position and the three normals.
        */
        Profile(const std::string & name, const Frame & frame);
        virtual ~Profile();

        // get
        std::string name() const;
        /**
        *	Access the near and far clipping distances.
        *	@return the clipping distances, near in x and far in y.
        */
        glm::vec2 clipping() const;
        float fov() const;
        float orthoZoom() const;
        float pixelAspect() const;
        bool orthographic() const;
        bool adaptiveProjection() const;
        bool adaptivePosition() const;
        glm::vec3 eye() const;
        glm::vec3 up() const;
        glm::vec3 right() const;
        glm::vec3 direction() const;
        glm::quat rotation() const;
        /**
        *	@return which basis lookat() builds, UpCrossDirection unless it was set
        */
        Hand hand() const;
        /**
        *	Access the size in pixels of the viewport the camera draws into.
        *	Zero until something that owns a viewport sets it.
        *	Camera::orthoFactorHorizontal() and Camera::orthoFactorVertical() return zero
        *	while it is zero.
        */
        glm::uvec2 size() const;

        // set
        void name(const std::string & name);
        /**
        *	The near and far clipping planes, as x and y, as clipping() returns them.
        */
        void clipping(const glm::vec2 & planes);
        void fov(float fov);
        void orthoZoom(float zoom);
        void pixelAspect(float aspect);
        void orthographic(bool ortho);
        /**
        *	Whether the far clipping distance should follow the scene bounds, so that
        *	objects do not clip out as they move away. The profile only stores the setting.
        *	The code that draws has to adjust the distance.
        *	@param adaptive the new adaptive projection setting.
        */
        void adaptiveProjection(bool adaptive);
        void adaptivePosition(bool adaptive);
        void eye(const glm::vec3 & position);
        /**
        *	Set one of the camera normals.
        *	The three normals and the rotation are separate state. Setting a normal does not
        *	recompute the rotation, and rotation() does not recompute the normals.
        *	lookat() and turn() write all four consistently.
        */
        void up(const glm::vec3 & up);
        void right(const glm::vec3 & right);
        void direction(const glm::vec3 & direction);
        /**
        *	Set the camera's rotation.
        *	The rotation tells how to transform the camera into the same space
        *	defined by its normals. The normals are the up, right, and direction
        *	vectors. The default coordinate system with no rotation is a right-
        *	handed system with right +x, up +y, and direction +z.
        *	@param rotation the new rotation value.
        */
        void rotation(const glm::quat & rotation);
        /**
        *	Turn the camera by a rotation composed onto the one it has, in its own axes.
        *	Unlike rotation(), this also recomputes the normals from the new rotation. A
        *	dolly or a truck after a turn therefore moves along the turned axes.
        *	@param local the rotation to compose, which need not be normalized.
        */
        void turn(const glm::quat & local);
        /**
        *	The rotation as a matrix. Camera::createView() transposes it into the view.
        *	Returns the basis matrix lookat() built until something else sets the rotation,
        *	and the quaternion cast to a matrix after that.
        */
        glm::mat4x4 orientation() const;
        void size(const glm::uvec2 & size);
        /**
        *	Choose which basis lookat() builds. Set it before calling lookat(). Setting it
        *	does not change the stored normals, and turn() and lookat() read it.
        *	@param hand the basis to build
        */
        void hand(Hand hand);

        /**
        *	Orient the camera to look at a point in space.
        *	The camera normals and rotation will be recalculated so the point
        *	will be in the center of the view.
        *	@param center the point in world space to focus the camera on.
        */
        void lookat(const glm::vec3 & center);
        /**
        *	Copy camera settings.
        *	@param profile the camera profile to copy settings from.
        */
        void clone(const Profile & profile);
        /**
        *	Assignment operator.
        *	Copy the camera settings from one profile and assign them to another,
        *	through clone().
        *	@param other the camera profile to copy settings from.
        *	@return the camera profile with the new settings copied to it.
        */
        Profile & operator = (const Profile & other);

 private:
        typedef enum CameraOptions : std::uint8_t {
            OPTION_ORTHOGRAPHIC = (1 << 1),
            OPTION_ADAPTIVE_PROJECTION = (1 << 2),
            OPTION_ADAPTIVE_POSITION = (1 << 3)
        } CameraOptions;

        std::string name_;
        glm::vec3 eye_;
        glm::vec3 direction_;
        glm::vec3 right_;
        glm::vec3 up_;
        Hand hand_;
        float orthoZoom_;
        float pixelAspect_;  // pixel aspect ratio w:h e.g. 4/3 = 1.33
        float near_;
        float far_;
        float fov_;  // y fov
        unsigned int options_;
        unsigned int size_[2];

        glm::quat rotation_;

        /**
        *	The basis matrix lookat() built. createView() uses it instead of casting the
        *	quaternion, so a view built through lookat() equals the one glm::lookAt builds,
        *	exactly. Casting the quaternion differs by about 2e-6 per element.
        *
        *	lookat() fills this cache. rotation() and turn() clear it, and so do
        *	Camera::pan() and Camera::tilt(), which call turn(). While it is clear,
        *	orientation() casts the quaternion. clone() copies the cache along with the
        *	rotation.
        *
        *	lookat() does not renormalize the up vector it derives, because glm::lookAt does
        *	not.
        */
        glm::mat4x4 basis_;
        bool basisValid_;
};

};  // namespace v3d::type::camera
