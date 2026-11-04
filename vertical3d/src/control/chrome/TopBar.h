/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/Component.h>
#include <api/ui/Container.h>
#include <api/ui/Interface.h>
#include <api/ui/paint/Type.h>

#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include <boost/json.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor::control::chrome {

/**
 * What sits on the menu bar's row beside its menus: a tab for each workspace, the project's name
 * with an amber dot while it has unsaved changes, and a button that opens the command search.
 *
 * They are a ui container, built as the editor runs and listed after the editor's own so they
 * are drawn over the menu bar's band, which takes no press beside its labels. The tabs follow
 * the menus, side by side in a recess, each a choice that is raised while its workspace is
 * shown. A tab sends workspace::show with the workspace's name, and the one shown is checked.
 * The name and the search sit at the right end. The search is a plate holding a magnifier, what
 * it does, and the key that opens it on a cap, under a button that takes the press.
 *
 * sync() builds the container again when the workspaces change, and otherwise moves it and
 * writes the name.
 **/
class TopBar final {
 public:
    /**
     * The container's name.
     **/
    static constexpr std::string_view NAME = "top-bar";

    /**
     * What the bar shows.
     **/
    struct State final {
        std::vector<std::string> workspaces;
        std::string shown;      /**< the workspace shown **/
        std::string project;    /**< the project's name **/
        bool dirty = false;     /**< whether it has changes that are not saved **/
        std::string searchKey;  /**< the key that opens the command search, or none **/
    };

    /**
     * What resolves the images a component names, such as an icon's.
     **/
    using Resolve = std::function<void(const boost::shared_ptr<v3d::ui::Component>&)>;

    /**
     * @param before the container this one goes before
     * @param type what the ui's text is drawn in, which sizes the bar's boxes
     * @param resolve what resolves the images of what the bar adds, which may be empty
     **/
    TopBar(const boost::shared_ptr<v3d::ui::Interface>& ui, const std::string& before, const v3d::ui::paint::Type& type,
        const Resolve& resolve);

    /**
     * Build the container again if the workspaces changed, put it in a band of the window, and
     * show the project's name and which workspace is shown.
     *
     * @param band where it goes: from the end of the menus to the right edge of the window, as
     *        tall as the menu bar
     **/
    void sync(const State& state, const glm::vec4& band);

    /**
     * @return the container a set of workspaces is drawn with, as Interface::add() reads it
     **/
    static boost::json::object document(const std::vector<std::string>& workspaces);

 private:
    /**
     * What the search button says.
     **/
    static constexpr const char* SEARCH = "Search commands";

    /**
     * How far the workspace tabs sit in from the bar's top and foot, as a share of its height.
     * Their plate is in half as far.
     **/
    static constexpr float TAB_INSET = 0.15F;

    /**
     * How tall the search's magnifier is, as a share of the tabs' height, and the unsaved
     * dot and a key's cap the same.
     **/
    static constexpr float ICON_SHARE = 0.6F;
    static constexpr float DOT_SHARE = 0.5F;
    static constexpr float CAP_SHARE = 0.75F;

    /**
     * Place the search at the right end of the bar. It leaves out what it does when its plate
     * would otherwise start before a limit.
     *
     * @param limit the leftmost the plate may start and still show what it does
     * @return where its plate starts
     **/
    float placeSearch(v3d::ui::Container* container, const State& state, const glm::vec4& band, float limit) const;

    /**
     * Set a component's box in window pixels.
     **/
    static void box(v3d::ui::Component* component, const glm::vec4& region);

    boost::shared_ptr<v3d::ui::Interface> ui_;
    std::string before_;
    v3d::ui::paint::Type type_;
    Resolve resolve_;
    // the workspaces the container was last built for
    std::vector<std::string> built_;
    bool added_ = false;
};

};  // namespace v3d::editor::control::chrome
