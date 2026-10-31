/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/Component.h>
#include <api/ui/Interface.h>
#include <vertical3d/src/view/AreaLayout.h>

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <boost/json.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::editor {

/**
 * The header and the tabs of each area, built as ui containers while the editor runs.
 *
 * Each area has a container named "area-" and its id. Its header is a menu bar that is not
 * docked, across the top of where the area's editor draws, holding one menu named for the kind
 * of editor it shows. The menu splits, joins and swaps the area, adds and closes tabs, and
 * maximises it. A maximise button sits at the header's right end. An area holding two or more
 * editors has a strip of tabs above its editor, one toggle button for each, with the chosen one
 * checked.
 *
 * Every command is in the "area" context. A header's commands carry the area's id as their
 * data, and a tab carries its editor's id.
 *
 * sync() builds the containers again when the areas or their editors change, and otherwise
 * moves them to where the areas are. They are put before the container that holds the
 * editor's menu bar, so a menu it drops is drawn over them.
 **/
class AreaChrome final {
 public:
    /**
     * The command context every header and tab sends in.
     **/
    static constexpr std::string_view CONTEXT = "area";

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
     * Build the containers again if the areas or their editors changed, and put each where its
     * area is. An area with no room hides its container.
     *
     * @param height how tall a header and a tab are
     **/
    void sync(const AreaLayout& layout, float height);

    /**
     * @return the id of the editor whose tab is at a point, or nothing when no tab is there
     **/
    std::optional<std::size_t> tabAt(const glm::vec2& point) const;

    /**
     * @return the containers one layout's areas are drawn with, as Interface::add() reads them
     **/
    static boost::json::object document(const AreaLayout& layout);

    /**
     * @return the name of an area's container
     **/
    static std::string name(std::size_t area);

    /**
     * @return what an editor is called on its tab: its camera for a viewport, and otherwise its
     *         kind with a capital
     **/
    static std::string label(const AreaLayout::Editor& editor);

 private:
    /**
     * @return what decides whether the containers have to be built again: each area's id, the
     *         kind of editor it shows, and its editors' ids and labels
     **/
    static std::string signature(const AreaLayout& layout);

    /**
     * Put one area's header, button and tabs where the area is.
     **/
    void place(const AreaLayout::Area& area, float height) const;

    boost::shared_ptr<v3d::ui::Interface> ui_;
    std::string before_;
    Resolve resolve_;
    std::string signature_;
    // the containers sync() last added, by name
    std::vector<std::string> held_;
};

};  // namespace v3d::editor
