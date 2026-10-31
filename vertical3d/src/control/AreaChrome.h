/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/Component.h>
#include <api/ui/Interface.h>
#include <api/ui/component/menu/Bar.h>
#include <vertical3d/src/selection/SelectMask.h>
#include <vertical3d/src/view/AreaLayout.h>
#include <vertical3d/src/view/AreaViews.h>
#include <vertical3d/src/view/ViewPort.h>

#include <cstddef>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <boost/json.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor::control {

/**
 * The header and the tabs of each area, built as ui containers while the editor runs.
 *
 * Each area has a container named "area-" and its id. Its header is a menu bar that is not
 * docked, across the top of where the area's editor draws, holding a menu named for the kind
 * of editor it shows. That menu changes the kind of editor shown, splits, joins and swaps the
 * area, adds and closes tabs, and maximises it. A maximise button sits at the header's right
 * end. An area holding two or more editors has a strip of tabs above its editor, one toggle
 * button for each, with the chosen one checked.
 *
 * A viewport's header has three more menus. One is named for the camera the view shows and
 * offers the fixed views and the projection switch. One is named for how the view shades the
 * scene and offers the shading, the fill of each side and the cull. Each marks what its view
 * holds. The last is named for the select mask, which every view shares, and offers each mask.
 *
 * The area menu's commands are in the "area" context, a viewport's camera and shading menus' in
 * the "view" context, and its mask menu's in the "select" context. Every command a header sends
 * carries the area's id as its data, and a tab carries its editor's id.
 *
 * sync() builds the containers again when the areas, their editors, or what a menu is named
 * for change, and otherwise moves them to where the areas are. They are put before the
 * container that holds the editor's menu bar, so a menu it drops is drawn over them.
 **/
class AreaChrome final {
 public:
    /**
     * The command context every header and tab sends in.
     **/
    static constexpr std::string_view CONTEXT = "area";

    /**
     * The context a viewport's camera and shading commands are in.
     **/
    static constexpr std::string_view VIEW_CONTEXT = "view";

    /**
     * A command that shows a fixed view, and the camera profile that view is.
     **/
    struct FixedView final {
        std::string command;
        std::string profile;
    };

    /**
     * A kind of editor an area can show, and what the header's Editor menu calls it.
     **/
    struct Kind final {
        std::string kind;
        std::string label;
    };

    /**
     * Resolves the images of a component that was just added.
     **/
    typedef std::function<void(const boost::shared_ptr<v3d::ui::Component>& component)> Resolve;

    /**
     * @param before the container the areas' containers go before
     * @param resolve what resolves the images of what is added, which may be empty
     **/
    AreaChrome(const boost::shared_ptr<v3d::ui::Interface>& ui, const std::string& before, const Resolve& resolve);

    /**
     * Build the containers again if the areas, their editors or the names of their menus
     * changed, and put each where its area is. Mark what each viewport holds and the select
     * mask on its header's menus. An area with no room hides its container.
     *
     * @param mask the select mask in force
     * @param height how tall a header and a tab are
     **/
    void sync(const view::AreaViews& views, selection::SelectMask mask, float height);

    /**
     * @return the id of the editor whose tab is at a point, or nothing when no tab is there
     **/
    std::optional<std::size_t> tabAt(const glm::vec2& point) const;

    /**
     * @return the containers the areas of the views' layout are drawn with, under a select mask,
     *         as Interface::add() reads them
     **/
    static boost::json::object document(const view::AreaViews& views, selection::SelectMask mask);

    /**
     * @return the name of an area's container
     **/
    static std::string name(std::size_t area);

    /**
     * @return what an editor is called on its tab: its camera for a viewport, and otherwise its
     *         kind with a capital
     **/
    static std::string label(const view::AreaLayout::Editor& editor);

    /**
     * @return the fixed views a viewport offers, each a command and the profile it shows
     **/
    static const std::vector<FixedView>& fixedViews();

    /**
     * @return what a view's shading menu is named: Points or Wireframe when the front of each
     *         triangle is drawn so, and otherwise Flat or Smooth
     **/
    static std::string shading(const view::ViewPort::Display& display);

    /**
     * @return each command that describes a view's state, as "view::" and its name, and whether
     *         the view holds that state: its camera and projection, its shading, fills and cull,
     *         and what it shows
     **/
    static std::vector<std::pair<std::string, bool>> marks(const view::ViewPort& view);

    /**
     * @return the kinds of editor an area can be changed to show, in the Editor menu's order
     **/
    static const std::vector<Kind>& kinds();

    /**
     * @return each kind's command, as "area::editor::" and the kind, and whether it is the kind
     *         of an editor
     **/
    static std::vector<std::pair<std::string, bool>> marks(const view::AreaLayout::Editor& editor);

    /**
     * @return each select mask's command, as "select::mask::" and its name, and whether it is
     *         the mask in force
     **/
    static std::vector<std::pair<std::string, bool>> marks(selection::SelectMask mask);

 private:
    /**
     * The four ways an area can be divided or met: as each command names it, and as a menu reads it
     * for a split and for a join or a swap.
     **/
    struct Way final {
        const char* command;
        const char* split;
        const char* across;
    };

    /** A choice on a view's menu: the end of its command, and what the menu names it. **/
    struct Choice final {
        const char* command;
        const char* label;
    };

    /** The masks a mask menu offers, in its order, and where a separator goes before one. **/
    struct Masked final {
        selection::SelectMask mask;
        bool rule;
    };

    /** A submenu of one of an area's commands, in each of the four ways. **/
    struct Ways final {
        const char* label;
        const char* command;
        bool across;   /**< whether the ways read as neighbours rather than as edges **/
    };

    static constexpr Way WAYS[] = {
        { .command = "left", .split = "Left", .across = "Left" },
        { .command = "right", .split = "Right", .across = "Right" },
        { .command = "top", .split = "Top", .across = "Above" },
        { .command = "bottom", .split = "Bottom", .across = "Below" },
    };

    static constexpr Choice FILLS[] = { { "points", "Points" }, { "lines", "Lines" }, { "fill", "Fill" } };
    static constexpr view::ViewPort::Fill FILLED[] = { view::ViewPort::Fill::Points, view::ViewPort::Fill::Lines, view::ViewPort::Fill::Fill };

    static constexpr Choice CULLS[] = { { "none", "None" }, { "front", "Front" }, { "back", "Back" }, { "both", "Both" } };
    static constexpr view::ViewPort::Cull CULLED[] = { view::ViewPort::Cull::None, view::ViewPort::Cull::Front, view::ViewPort::Cull::Back, view::ViewPort::Cull::Both };

    static constexpr Masked MASKS[] = {
        { .mask = selection::SelectMask::Object, .rule = false }, { .mask = selection::SelectMask::Mesh, .rule = false },
        { .mask = selection::SelectMask::Curve, .rule = false }, { .mask = selection::SelectMask::Camera, .rule = false },
        { .mask = selection::SelectMask::Light, .rule = false }, { .mask = selection::SelectMask::Vertex, .rule = true },
        { .mask = selection::SelectMask::Edge, .rule = false }, { .mask = selection::SelectMask::Face, .rule = false },
        { .mask = selection::SelectMask::Handle, .rule = false },
    };

    /**
     * @return a menu item that sends an area's command
     **/
    static boost::json::object item(const std::string& label, const std::string& command, std::size_t area);

    /**
     * @return a check or radio item that sends a view's command for an area
     **/
    static boost::json::object mark(const std::string& label, const char* type, const std::string& command, std::size_t area);

    /**
     * @return a submenu of radio items, one for each choice, sending a view's command for an area
     **/
    static boost::json::object choices(const char* label, const std::string& prefix, std::span<const Choice> each, std::size_t area);

    /**
     * @return a kind with a capital, as a menu names it
     **/
    static std::string capital(const std::string& kind);

    /**
     * @return a viewport's mask menu, named for the mask in force
     **/
    static boost::json::object maskMenu(selection::SelectMask mask, std::size_t area);

    /**
     * @return a viewport's camera menu and its shading menu, named for what the view shows
     **/
    static boost::json::array viewMenus(const view::ViewPort& view, std::size_t area);

    /**
     * @return the submenu, its items sending the command for one area
     **/
    static boost::json::object ways(const Ways& spec, std::size_t area);

    /**
     * Check or clear the items of a menu bar that send each command, as a list of marks says.
     **/
    static void check(v3d::ui::component::menu::Bar* bar, const std::vector<std::pair<std::string, bool>>& marks);

    /**
     * Set a component's box in window pixels.
     **/
    static void box(v3d::ui::Component* component, const glm::vec4& region);

    /**
     * @return what decides whether the containers have to be built again. That is each area's
     *         id, the kind of editor it shows, its editors' ids and labels, and the names of its
     *         view's menus and of the mask
     **/
    static std::string signature(const view::AreaViews& views, selection::SelectMask mask);

    /**
     * Put one area's header, button and tabs where the area is, and mark what its view holds
     * and the mask.
     *
     * @param view the view the area shows, or null when it shows another kind of editor
     **/
    void place(const view::AreaLayout::Area& area, const view::ViewPort* view, selection::SelectMask mask, float height) const;

    boost::shared_ptr<v3d::ui::Interface> ui_;
    std::string before_;
    Resolve resolve_;
    std::string signature_;
    // the containers sync() last added, by name
    std::vector<std::string> held_;
};

};  // namespace v3d::editor::control
