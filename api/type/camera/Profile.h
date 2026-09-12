/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

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
        *	Which way round lookat() crosses its normals, and so which world direction
        *	ends up on the right of the screen. Both are right handed bases and both
        *	render; they mirror each other horizontally, which also reverses the winding
        *	a front face presents - see ADR-0012.
        *
        *	A profile in this tree is UpCrossDirection and says nothing, which is what
        *	every camera here has always meant. DirectionCrossUp exists for an
        *	application whose geometry was authored against glm::lookAt: adopting this
        *	camera is otherwise a decision about that application's whole renderer.
        */
        enum class Hand {
                UpCrossDirection,       /**< right = up x direction **/
                DirectionCrossUp        /**< right = direction x up, which glm::lookAt builds **/
        };

        explicit Profile(const std::string & name);
        /**
        *	Construct a profile with an explicit basis.
        *	The basis vectors are taken as given and the rotation is left at identity,
        *	so a profile built this way is oriented by a later lookat() or rotation().
        *	@param name the profile name.
        *	@param eye the camera position.
        *	@param up the camera up vector.
        *	@param right the camera right vector.
        *	@param direction the direction of view.
        */
        Profile(const std::string & name, const glm::vec3 & eye, const glm::vec3 & up,
            const glm::vec3 & right, const glm::vec3 & direction);
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
        *	Zero until something that owns a viewport sets it, which is what
        *	Camera::orthoFactorHorizontal() and ::orthoFactorVertical() guard against.
        */
        glm::uvec2 size() const;

        // set
        void name(const std::string & name);
        void clipping(float near, float far);
        void fov(float fov);
        void orthoZoom(float zoom);
        void pixelAspect(float aspect);
        void orthographic(bool ortho);
        /**
        *	Whether the far clipping distance should follow the scene bounds, so that
        *	objects do not clip out as they move away. The profile records the setting
        *	and nothing more - adjusting the distance is left to whoever draws.
        *	@param adaptive the new adaptive projection setting.
        */
        void adaptiveProjection(bool adaptive);
        void adaptivePosition(bool adaptive);
        void eye(const glm::vec3 & position);
        /**
        *	Set one of the camera normals.
        *	The three normals and the rotation are independent state - setting a normal
        *	does not recompute the rotation, and rotating does not recompute the normals.
        *	lookat() is the one call that writes all four consistently.
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
        void size(unsigned int width, unsigned int height);
        /**
        *	Choose which basis lookat() builds. It changes nothing until lookat() runs,
        *	since the normals are state rather than a derivation, so set it before.
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
        *	Copy the camera settings from one profile and assign them to another.
        *	This is just the profile method packaged in an operator.
        *	@param p the camera profile to copy settings from.
        *	@return the camera profile with the new settings copied to it.
        */
        Profile & operator = (const Profile & p);

 protected:
        friend class Camera;

        typedef enum CameraOptions {
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
};

};  // namespace v3d::type::camera
