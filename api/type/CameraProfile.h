/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <string>

#include <glm/glm.hpp>
#include <glm/ext/quaternion_float.hpp>

namespace v3d::type {
    /**
     * A class containing the common camera settings.
     */
    class CameraProfile {
     public:
            explicit CameraProfile(const std::string & name);
            virtual ~CameraProfile();

            void clipping(float near, float far);
            void eye(const glm::vec3 & position);

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
            void clone(const CameraProfile & profile);
            /**
            *	Assignment operator.
            *	Copy the camera settings from one profile and assign them to another.
            *	This is just the profile method packaged in an operator.
            *	@param p the camera profile to copy settings from.
            *	@return the camera profile with the new settings copied to it.
            */
            CameraProfile & operator = (const CameraProfile & p);

     protected:
            friend class Camera;

            typedef enum CameraOptions {
                OPTION_ORTHOGRAPHIC = (1 << 1),
                OPTION_DEFAULT = (1 << 2)
            } CameraOptions;

            std::string name_;
            glm::vec3 eye_;
            glm::vec3 direction_;
            glm::vec3 right_;
            glm::vec3 up_;
            float orthoZoom_;
            float pixelAspect_;  // pixel aspect ratio w:h e.g. 4/3 = 1.33
            float near_;
            float far_;
            float fov_;  // y fov
            unsigned int options_;
            unsigned int size_[2];

            glm::quat rotation_;
    };

};  // namespace v3d::type
