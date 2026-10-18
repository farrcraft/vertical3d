/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/camera/ArcBall.h>
#include <vertical3d/src/view/ViewPort.h>

#include <cstdint>
#include <string>

#include "Tool.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 * The CameraControlTool provides interactive manipulation of a Camera through a ViewPort.
 *
 * The modifier held chooses which of the three moves a drag performs. The tool has no
 * viewport of its own. It drives the view the cursor was in when the drag started, so one
 * tool serves the camera of every view.
 */
class CameraControlTool final : public Tool {
 public:
    CameraControlTool();

    typedef enum CameraMode : std::uint8_t {
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
     * Where the cursor is within the view. The arcball and the deltas are measured from
     * the view's origin, which is not the window's.
     **/
    glm::vec2 local(const glm::vec2& position) const;

    glm::vec2 last_;
    CameraMode mode_;
    bool dragging_;
    boost::shared_ptr<ViewPort> view_;
    v3d::type::camera::ArcBall arcball_;
};

};  // namespace v3d::editor
