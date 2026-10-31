/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/Component.h>
#include <api/ui/Interface.h>
#include <vertical3d/src/selection/SelectMask.h>
#include <vertical3d/src/view/AreaLayout.h>
#include <vertical3d/src/view/AreaViews.h>
#include <vertical3d/src/view/ViewPort.h>

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <boost/json.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::editor {

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
     * changed, put each where its area is, and mark what each viewport holds and the select
     * mask on its header's menus. An area with no room hides its container.
     *
     * @param mask the select mask in force
     * @param height how tall a header and a tab are
     **/
    void sync(const AreaViews& views, SelectMask mask, float height);

    /**
     * @return the id of the editor whose tab is at a point, or nothing when no tab is there
     **/
    std::optional<std::size_t> tabAt(const glm::vec2& point) const;

    /**
     * @return the containers the areas of the views' layout are drawn with, under a select mask,
     *         as Interface::add() reads them
     **/
    static boost::json::object document(const AreaViews& views, SelectMask mask);

    /**
     * @return the name of an area's container
     **/
    static std::string name(std::size_t area);

    /**
     * @return what an editor is called on its tab: its camera for a viewport, and otherwise its
     *         kind with a capital
     **/
    static std::string label(const AreaLayout::Editor& editor);

    /**
     * @return the fixed views a viewport offers, each a command and the profile it shows
     **/
    static const std::vector<FixedView>& fixedViews();

    /**
     * @return what a view's shading menu is named: Points or Wireframe when the front of each
     *         triangle is drawn so, and otherwise Flat or Smooth
     **/
    static std::string shading(const ViewPort::Display& display);

    /**
     * @return each command that describes a view's state, as "view::" and its name, and whether
     *         the view holds that state: its camera and projection, its shading, fills and cull,
     *         and what it shows
     **/
    static std::vector<std::pair<std::string, bool>> marks(const ViewPort& view);

    /**
     * @return the kinds of editor an area can be changed to show, in the Editor menu's order
     **/
    static const std::vector<Kind>& kinds();

    /**
     * @return each kind's command, as "area::editor::" and the kind, and whether it is the kind
     *         of an editor
     **/
    static std::vector<std::pair<std::string, bool>> marks(const AreaLayout::Editor& editor);

    /**
     * @return each select mask's command, as "select::mask::" and its name, and whether it is
     *         the mask in force
     **/
    static std::vector<std::pair<std::string, bool>> marks(SelectMask mask);

 private:
    /**
     * @return what decides whether the containers have to be built again: each area's id, the
     *         kind of editor it shows, its editors' ids and labels, and the names of its view's
     *         menus and of the mask
     **/
    static std::string signature(const AreaViews& views, SelectMask mask);

    /**
     * Put one area's header, button and tabs where the area is, and mark what its view holds
     * and the mask.
     *
     * @param view the view the area shows, or null when it shows another kind of editor
     **/
    void place(const AreaLayout::Area& area, const ViewPort* view, SelectMask mask, float height) const;

    boost::shared_ptr<v3d::ui::Interface> ui_;
    std::string before_;
    Resolve resolve_;
    std::string signature_;
    // the containers sync() last added, by name
    std::vector<std::string> held_;
};

};  // namespace v3d::editor
