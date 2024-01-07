/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "Tool.h"
#include "ViewPort.h"
#include "../api/type/ArcBall.h"

#include <boost/shared_ptr.hpp>

/**
 * The CameraControlTool provides interactive manipulation of a Camera through a ViewPort.
 */
class CameraControlTool : public v3d::Tool {
 public:
        explicit CameraControlTool(const boost::shared_ptr<v3d::ViewPort> & view);

        typedef enum CameraMode {
            CAMERA_MODE_ZOOM,
            CAMERA_MODE_TRUCK,
            CAMERA_MODE_PAN,
            CAMERA_MODE_NONE
        } CameraMode;

        // tool overrides
        void activate(const std::string & name);
        void deactivate(const std::string & name);

        // mouse event listener overrides
        void motion(unsigned int x, unsigned int y);
        void buttonPressed(unsigned int button);
        void buttonReleased(unsigned int button);

        // these each happen when there is a motion event and the primary mouse button is pressed
        void zoom(const glm::vec2 & position);
        void truck(const glm::vec2 & position);
        void pan(const glm::vec2 & position);

 private:
        glm::vec2 last_;
        CameraMode mode_;
        boost::shared_ptr<v3d::ViewPort> view_;
        v3d::type::ArcBall arcball_;
};

