/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/camera/ArcBall.h>
#include <vertical3d/src/view/ViewPort.h>

#include <cstdint>
#include <functional>
#include <string>

#include "Tool.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor::tool {

/**
 * The CameraControl provides interactive manipulation of a Camera through a ViewPort.
 *
 * A move is chosen two ways. A held move lasts while its modifier is down, and a latched move
 * lasts until it is latched again or released. A held move wins while it is held, and the
 * latched one comes back when it is let go. A drag of the primary button performs the move in
 * force.
 *
 * | Move | Perspective view | Orthographic view |
 * |---|---|---|
 * | Zoom | Widens or narrows the angle of view | Changes the half height it shows |
 * | Dolly | Moves the eye along the view direction | Changes the half height, as zoom does |
 * | Truck | Moves the eye sideways and up or down | The same, scaled to the view |
 * | Pan | Turns the camera with an arcball | Nothing |
 * | Pitch | Turns the camera about its own x axis | Nothing |
 *
 * An orthographic view is not turned, because the fixed views are orthographic and a turn
 * would leave them showing something else. The tool has no viewport of its own. It drives the
 * view the cursor was in when the drag started, so one tool serves the camera of every view.
 */
class CameraControl final : public Tool {
 public:
    CameraControl();

    typedef enum CameraMode : std::uint8_t {
        CAMERA_MODE_ZOOM,
        CAMERA_MODE_TRUCK,
        CAMERA_MODE_PAN,
        CAMERA_MODE_DOLLY,
        CAMERA_MODE_PITCH,
        CAMERA_MODE_NONE
    } CameraMode;

    /**
     * The narrowest and the widest angle of view a perspective zoom leaves, in degrees.
     **/
    static constexpr float narrowest = 1.0F;
    static constexpr float widest = 170.0F;

    // tool overrides: the held move, by name
    void activate(const std::string& name) override;
    void deactivate(const std::string& name) override;

    /**
     * Latch a move by name, or release it when it is the move already latched, as choosing a
     * checked menu item again unchecks it.
     **/
    void latch(const std::string& name);

    /**
     * Release the latched move, if there is one.
     **/
    void unlatch() noexcept;

    /**
     * @return the move latched, which is CAMERA_MODE_NONE when there is none
     **/
    CameraMode latched() const noexcept;
    void motion(const glm::vec2& position) override;
    void button(unsigned int button, bool pressed, const glm::vec2& position) override;

    /**
     * The view a drag will be applied to, set as the cursor moves between viewports.
     * The arcball is given the view's size, because it maps a click onto a sphere the
     * size of the view rather than of the window.
     **/
    void view(const boost::shared_ptr<view::ViewPort>& view) override;

    /**
     * @return the view the tool is driving, which may be null
     **/
    boost::shared_ptr<view::ViewPort> view() const;

    /**
     * What a drag did to a view that looked through a scene camera: freed the view before its
     * first move, moved the view locked to the camera, or ended a drag that did.
     **/
    enum class Navigated : std::uint8_t { Freed, Moved, Ended };

    /**
     * What is told what a drag did to a view through a scene camera.
     **/
    using Watch = std::function<void(view::ViewPort& view, Navigated what)>;

    /**
     * Be told what each drag does to a view through a scene camera. A view that is not locked to
     * its camera is freed before the drag moves it, so the drag moves the view and not the
     * camera. A locked view is told of after each move, so the camera can follow it.
     **/
    void watch(const Watch& watch);

    /**
     * @return which move a drag performs: the held move while there is one, and the latched
     *         move otherwise
     **/
    CameraMode mode() const noexcept;

    /**
     * @return whether a drag is under way
     **/
    bool dragging() const noexcept override;

    /**
     * @return whether a move is held or latched, which makes a press drive the camera
     **/
    bool takes(const glm::vec2& position) const override;

    // these each happen when there is a motion event and the primary mouse button is pressed
    void zoom(const glm::vec2& position);
    void dolly(const glm::vec2& position);
    void truck(const glm::vec2& position);
    void pan(const glm::vec2& position);
    void pitch(const glm::vec2& position);

 private:
    /**
     * World units a perspective camera moves for each pixel the mouse is dragged.
     **/
    static constexpr float perspectiveFactor = 0.125F;

    /**
     * How far a perspective zoom changes the angle of view, in degrees for each pixel of the drag.
     **/
    static constexpr float fovFactor = 0.25F;

    /**
     * How far a pitch turns the camera, in radians for each pixel of the drag.
     **/
    static constexpr float pitchFactor = 0.005F;

    /** The move a command names, or none. **/
    static CameraMode named(const std::string& name);

    /**
     * Where the cursor is within the view. The arcball and the deltas are measured from
     * the view's origin, which is not the window's.
     **/
    glm::vec2 local(const glm::vec2& position) const;

    glm::vec2 last_;
    CameraMode mode_;
    CameraMode latched_;
    bool dragging_;
    boost::shared_ptr<view::ViewPort> view_;
    v3d::type::camera::ArcBall arcball_;
    Watch watch_;
};

};  // namespace v3d::editor::tool
