/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "Tool.h"
#include "../view/ViewPort.h"

#include "../../../api/type/ArcBall.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

    /**
     * The CameraControlTool provides interactive manipulation of a Camera through a ViewPort.
     *
     * Which of the three moves a drag performs is chosen by the modifier held. The tool
     * has no viewport of its own: the view it drives is the one the cursor was in when the
     * drag started, so a four way split is four cameras and one tool.
     */
    class CameraControlTool final : public Tool {
     public:
        CameraControlTool();

        typedef enum CameraMode {
            CAMERA_MODE_ZOOM,
            CAMERA_MODE_TRUCK,
            CAMERA_MODE_PAN,
            CAMERA_MODE_NONE
        } CameraMode;

        // tool overrides
        void activate(const std::string& name) override;
        void deactivate(const std::string& name) override;
        void motion(const glm::vec2& position) override;
        void button(unsigned int button, bool pressed, const glm::vec2& position) override;

        /**
         * The view a drag will be applied to, set as the cursor moves between viewports.
         * The arcball is given the view's size, because it maps a click onto a sphere the
         * size of the view rather than of the window.
         **/
        void view(const boost::shared_ptr<ViewPort>& view);

        /**
         * @return the view the tool is driving, which may be null
         **/
        boost::shared_ptr<ViewPort> view() const;

        /**
         * @return which move a drag performs
         **/
        CameraMode mode() const noexcept;

        /**
         * @return whether a drag is under way
         **/
        bool dragging() const noexcept;

        // these each happen when there is a motion event and the primary mouse button is pressed
        void zoom(const glm::vec2& position);
        void truck(const glm::vec2& position);
        void pan(const glm::vec2& position);

     private:
        /**
         * Where the cursor is within the view, which is what the arcball and the deltas are
         * measured in - the window origin is not the view's.
         **/
        glm::vec2 local(const glm::vec2& position) const;

        glm::vec2 last_;
        CameraMode mode_;
        bool dragging_;
        boost::shared_ptr<ViewPort> view_;
        v3d::type::ArcBall arcball_;
    };

};  // namespace v3d::editor
