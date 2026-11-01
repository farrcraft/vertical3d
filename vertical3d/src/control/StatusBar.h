/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/paint/Text.h>
#include <vertical3d/src/scene/Components.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Timeline.h>
#include <vertical3d/src/selection/SelectMask.h>
#include <vertical3d/src/tool/CameraControl.h>
#include <vertical3d/src/tool/Select.h>
#include <vertical3d/src/tool/Transform.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
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
        std::string name;       /**< the button's name, which shows when its icon does not **/
        std::string action;
        std::string icon;       /**< the icon of a mouse with the button lit **/
        std::string modifier;   /**< a key held with the button for this action, or empty **/
    };

    /**
     * How the frame rate stands against the monitor's.
     **/
    enum class Health : std::uint8_t { Good, Low, Poor };

    std::vector<Button> buttons;
    std::string selection;
    std::string frame;
    std::string rate;   /**< empty before a frame has been measured **/
    Health health = Health::Good;
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
     * mode's handles move, turn or scale what is selected. While it only selects, the keys held
     * with it follow: Shift adds or removes, Alt dollies and Ctrl trucks. The middle button turns
     * a perspective camera, and does nothing in an orthographic view, so it is left out there.
     *
     * @param perspective whether the active view has a perspective camera
     **/
    static std::vector<Status::Button> buttons(const tool::CameraControl& camera, const tool::Transform& transform, const tool::Select& select,
        bool perspective);

    /**
     * @return what is selected under a mask, or "Nothing selected". Under a mask that selects
     *         whole items, one item is named, with a mesh's vertices and faces, and several are
     *         counted by kind. Under one that selects parts, the parts are counted
     **/
    static std::string selection(const scene::Scene& scene, selection::SelectMask mask);

    /**
     * @return the frame the timeline is at, of its last: "Frame 24 of 120"
     **/
    static std::string frame(const scene::Timeline& timeline);

    /**
     * How fast frames are drawn, and how fast the monitor shows them.
     **/
    struct Timing final {
        std::uint64_t mean = 0;   /**< the mean time a frame takes in nanoseconds, or 0 before one has been measured **/
        double refresh = 0.0;     /**< the monitor's rate in hertz, or 0 when it is not known, which is taken as 60 **/
    };

    /**
     * @return how the frame rate stands against the monitor's: good at nine tenths of it or more,
     *         poor below half of it, and low between. No time measured is good
     **/
    static Status::Health health(const Timing& timing);

    /**
     * @return a count of things with its noun, singular for one: "1 face", "3 vertices"
     **/
    static std::string count(std::size_t number, const std::string& one, const std::string& many);

    /**
     * @return the frame rate a mean frame time makes, as "60 fps", or nothing for no time
     **/
    static std::string rate(std::uint64_t nanoseconds);

    /**
     * @return how wide the frame rate's slot is: as wide as a rate of three of the widest digit,
     *         or as the rate itself when that is wider. The rate is drawn at the slot's right, so
     *         the text before the slot stays where it is as the rate changes
     **/
    static float rateRoom(const v3d::ui::paint::Measure& measure, const std::string& rate);

 private:
    /**
     * What a frame rate's number is followed by.
     **/
    static constexpr std::string_view RATE_UNIT = " fps";

    /**
     * How many digits the frame rate's slot has room for before it grows.
     **/
    static constexpr std::size_t RATE_DIGITS = 3;

    /**
     * How many nanoseconds there are in a second.
     **/
    static constexpr double SECOND = 1e9;

    /**
     * The icons of a mouse with its left and its middle button lit.
     **/
    static constexpr std::string_view LEFT_ICON = "icon:mouse-left";
    static constexpr std::string_view MIDDLE_ICON = "icon:mouse-middle";

    /**
     * What separates the parts of one item's description, or of a count by kind.
     **/
    static constexpr std::string_view PART_GAP = " · ";

    /**
     * The monitor's rate when it is not known, and the shares of it that are good and poor.
     **/
    static constexpr double ASSUMED_REFRESH = 60.0;
    static constexpr double GOOD_SHARE = 0.9;
    static constexpr double POOR_SHARE = 0.5;

    /**
     * @return what selected items are: a name, with a mesh's counts, for one, and counts by kind
     *         for several
     **/
    static std::string items(const scene::Items& selected);

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
