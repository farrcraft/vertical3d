/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/engine/CameraProfiles.h>
#include <api/engine/Settings.h>
#include <api/event/Event.h>
#include <api/log/Logger.h>
#include <api/type/camera/Profile.h>
#include <api/ui/shell/FileChooser.h>
#include <api/ui/shell/Prompt.h>
#include <vertical3d/src/control/chrome/Notices.h>
#include <vertical3d/src/control/Playback.h>
#include <vertical3d/src/control/chrome/Pointer.h>
#include <vertical3d/src/control/RenderView.h>
#include <vertical3d/src/control/TimelineEditor.h>
#include <vertical3d/src/control/Workspaces.h>
#include <vertical3d/src/document/Document.h>
#include <vertical3d/src/selection/SelectMask.h>
#include <vertical3d/src/tool/CameraControl.h>
#include <vertical3d/src/tool/Select.h>
#include <vertical3d/src/tool/Transform.h>
#include <vertical3d/src/view/AreaLayout.h>
#include <vertical3d/src/view/AreaViews.h>
#include <vertical3d/src/view/ViewPort.h>

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>

#include <boost/shared_ptr.hpp>

namespace v3d::editor::app {

/**
 * What the editor's commands act on: the document, the areas and their views, the tools and
 * the controllers. The controller fills it in at start. A command reads what it needs from it
 * when it runs, so the commands can be registered before any of it exists.
 **/
struct Context final {
    /**
     * Log what an operation said, and show it as a notice.
     **/
    void report(const document::Outcome& outcome) const;

    /**
     * Log something the user asked for that could not be done, and show it as a notice.
     **/
    void refuse(const std::string& message) const;

    /**
     * @return the view the pointer last chose, or nothing before there is one
     **/
    boost::shared_ptr<view::ViewPort> activeView() const;

    /**
     * @return the area a command acts on: the one whose id it carries, or the active area
     **/
    std::size_t areaOf(const v3d::event::Event& event) const;

    /**
     * Show an editor of a kind: the first one an area holds. When no area holds one, it is shown
     * in a window of its own when windowed asks for one and a window can be opened, and as a new
     * tab of the active area otherwise.
     **/
    void show(std::string_view kind, bool windowed = false) const;

    /**
     * Run something that needs a view's camera, with the active view's. Without an active
     * view it says why nothing happened.
     * @param what what needs it, for the log
     **/
    void atView(const std::string& what, const std::function<void(const v3d::type::camera::Profile&)>& run) const;

    /**
     * Run an operation on the selected parts, when the select mask selects the parts it works
     * on. Under any other mask it says so and does nothing.
     **/
    void masked(selection::SelectMask mask, const std::string& name, const std::function<document::Outcome()>& operation) const;

    /**
     * The camera a render sees through: the one selected scene camera, or the active view.
     **/
    v3d::type::camera::Profile shot() const;

    /**
     * @return the current project's directory, as its record in the settings is keyed
     **/
    std::string projectKey() const;

    // the directory relative asset paths are resolved against, which moya.exe is built in
    std::string path;
    boost::shared_ptr<v3d::log::Logger> logger;
    // the player's settings, which keep the workspaces. Nothing without the config feature
    boost::shared_ptr<v3d::engine::Settings> settings;
    boost::shared_ptr<document::Document> document;
    boost::shared_ptr<v3d::engine::CameraProfiles> profiles;
    // the workspaces kept, and the layout of the one shown
    boost::shared_ptr<control::Workspaces> workspaces;
    boost::shared_ptr<view::AreaLayout> layout;
    boost::shared_ptr<view::AreaViews> views;
    // what commands said, shown at the foot of the active area
    boost::shared_ptr<control::chrome::Notices> notices;
    boost::shared_ptr<tool::CameraControl> cameraTool;
    boost::shared_ptr<tool::Select> selectTool;
    boost::shared_ptr<tool::Transform> transformTool;
    boost::shared_ptr<control::chrome::Pointer> pointer;
    boost::shared_ptr<control::Playback> playback;
    // every timeline editor, which share the chosen keys
    boost::shared_ptr<control::TimelineEditor> timeline;
    boost::shared_ptr<control::RenderView> renderView;
    boost::shared_ptr<v3d::ui::shell::FileChooser> chooser;
    boost::shared_ptr<v3d::ui::shell::Prompt> prompt;
    // run after the areas change, so the views take their regions again
    std::function<void()> areasChanged;
    // shows an editor of a kind in a window of its own, the one already open or a new one, and
    // says whether it could
    std::function<bool(std::string_view kind)> window;
};

};  // namespace v3d::editor::app
