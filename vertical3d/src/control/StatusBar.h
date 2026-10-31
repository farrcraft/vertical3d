/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vertical3d/src/scene/Components.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/selection/SelectMask.h>
#include <vertical3d/src/tool/CameraControl.h>
#include <vertical3d/src/tool/Select.h>
#include <vertical3d/src/tool/Transform.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace v3d::editor::control {

/**
 * What the status bar along the foot of the window says.
 *
 * On the left it says what each mouse button does in the active view, under the tools and the
 * modifiers held. On the right it says what is selected, the frame the timeline is at, and the
 * frame rate.
 **/
struct Status final {
    /**
     * A mouse button and what a drag or a click of it does.
     **/
    struct Button final {
        std::string name;
        std::string action;
    };

    std::vector<Button> buttons;
    std::string selection;
    std::string frame;
    std::string rate;   /**< empty before a frame has been measured **/
};

/**
 * Says what goes in the status bar.
 **/
class StatusBar final {
 public:
    /**
     * What the left and middle buttons do.
     *
     * The left button drives the camera while a camera move is held or latched. Otherwise it
     * selects, or adds to the selection and takes from it while that is held, and the transform
     * mode's handles move, turn or scale what is selected. The middle button turns a perspective
     * camera, and does nothing in an orthographic view, so it is left out there.
     *
     * @param perspective whether the active view has a perspective camera
     **/
    static std::vector<Status::Button> buttons(const tool::CameraControl& camera, const tool::Transform& transform, const tool::Select& select,
        bool perspective);

    /**
     * @return what is selected under a mask: a count of items under a mask that selects whole
     *         items, and of parts under one that selects parts, or "Nothing selected"
     **/
    static std::string selection(const scene::Scene& scene, selection::SelectMask mask);

    /**
     * @return a count of things with its noun, singular for one: "1 face", "3 vertices"
     **/
    static std::string count(std::size_t number, const std::string& one, const std::string& many);

    /**
     * @return the frame rate a mean frame time makes, as "60 fps", or nothing for no time
     **/
    static std::string rate(std::uint64_t nanoseconds);

 private:
    /**
     * How many nanoseconds there are in a second.
     **/
    static constexpr double SECOND = 1e9;

    /**
     * @return what a camera move is called on the status bar, or nothing for no move
     **/
    static std::optional<std::string> cameraMove(tool::CameraControl::CameraMode mode);

    /**
     * @return what a transform mode's handles do to the selection, or nothing for no handles
     **/
    static std::optional<std::string> handles(tool::Transform::Mode mode);

    /**
     * @return how many parts of one kind are selected in an item. An edge counts once, though
     *         either of its halves or both may be selected
     **/
    static std::size_t parts(const scene::Scene& scene, const scene::Item& item, scene::Component kind);
};

};  // namespace v3d::editor::control
