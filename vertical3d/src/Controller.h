/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <api/engine/CameraProfiles.h>
#include <api/engine/Engine.h>
#include <api/event/kind/MouseButton.h>
#include <api/event/kind/MouseMotion.h>
#include <api/event/kind/WindowResize.h>
#include <api/shell/Shell.h>
#include <api/type/camera/Profile.h>
#include <api/ui/component/Toolbar.h>
#include <api/ui/component/menu/Bar.h>
#include <api/ui/shell/FileChooser.h>
#include <api/ui/shell/Keyboard.h>
#include <api/ui/shell/Prompt.h>
#include <vertical3d/src/control/AreaChrome.h>
#include <vertical3d/src/control/CommandSearch.h>
#include <vertical3d/src/control/Notices.h>
#include <vertical3d/src/control/Workspaces.h>
#include <vertical3d/src/control/Playback.h>
#include <vertical3d/src/control/Pointer.h>
#include <vertical3d/src/control/RenderWindow.h>
#include <vertical3d/src/control/StatusBar.h>
#include <vertical3d/src/document/Document.h>
#include <vertical3d/src/selection/SelectMask.h>
#include <vertical3d/src/tool/CameraControlTool.h>
#include <vertical3d/src/tool/SelectTool.h>
#include <vertical3d/src/tool/ToolRouter.h>
#include <vertical3d/src/tool/TransformTool.h>
#include <vertical3d/src/view/AreaLayout.h>
#include <vertical3d/src/view/AreaViews.h>
#include <vertical3d/src/view/ViewPort.h>

#include <functional>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <entt/entt.hpp>

namespace v3d::editor {

class Renderer;

/**
 * The editor application.
 *
 * It builds the document, the views, the tools and the ui at start, and joins them. The
 * document holds what is edited and every operation on it. Playback, the Render menu and the
 * routing of the mouse each have a controller of their own. What is left here is the table of
 * commands, which turns each menu item and binding into a call, and the dialogs that ask the
 * user before a project is replaced.
 **/
class Controller final : public v3d::engine::Engine {
 public:
    /**
     * @param path the directory relative asset paths are resolved against
     **/
    explicit Controller(const std::string& path);

    /**
     **/
    bool render() override;

    /**
     * Offer every event to the ui keyboard before the bindings map it. While the command
     * search is up, the up and down arrows move its choice and Escape takes it down, first.
     *
     * Only the keyboard goes this way. The ui's share of the mouse is decided by the pointer,
     * from the mapped view::drag command.
     **/
    bool onEvent(const SDL_Event& event) override;

    /**
     * Autosave the document, render the render window again when the scene has changed, and
     * show a batch's progress in the caption.
     **/
    bool tick(unsigned int delta) override;

    /**
     * Play the timeline.
     **/
    bool simulate(float step) override;

    /**
     * The cursor moved. The pointer routes it to the ui or the tools, and chooses the active
     * view. Ctrl snaps a held divider to tenths.
     **/
    void handleMotion(const v3d::event::kind::MouseMotion& event);

    /**
     * A mouse button changed. A right press puts a held divider back, and a left press's clicks
     * are kept for the drag command that follows it.
     **/
    void handleButton(const v3d::event::kind::MouseButton& event);

    /**
     * Show the pointer as a resize cursor over a divider that is lit, and as an arrow elsewhere.
     **/
    void syncPointerShape();

    /**
     * Register the commands that change how a view draws: its camera, its projection, its
     * shading, fills and cull, and what it shows.
     **/
    void registerViewCommands();

    /**
     * Register the commands that change the areas: split, join, swap, maximise and the tabs.
     **/
    void registerAreaCommands();

    /**
     * @return the area a command acts on: the one whose id it carries, or the active area
     **/
    std::size_t areaOf(const v3d::event::Event& event) const;

    /**
     * Give the views their regions again after a change to the areas.
     **/
    void areasChanged();

    /**
     * @return the current project's directory, as its record in the settings is keyed
     **/
    std::string projectKey() const;

    /**
     * Show a workspace: the current project's own arrangement of it, or the workspace as it is
     * kept when the project has none.
     *
     * @return whether there is a workspace of that name that could be shown
     **/
    bool showWorkspace(const std::string& name);

    /**
     * Record the current project's arrangement in the workspace shown, and save the settings.
     **/
    void recordProject();

    /**
     * Show the workspace a project that was just opened or saved under a new name was last shown
     * in, or record the arrangement it has now when it has no record.
     **/
    void projectChanged();

    /**
     * Write the workspaces and the projects' records to the settings.
     **/
    void saveSettings();

    /**
     * Show the workspace a step after the one shown, wrapping at either end.
     **/
    void switchWorkspace(int step);

    /**
     * Write the workspace shown, as it is arranged, to a file the user chooses.
     **/
    void exportWorkspace();

    /**
     * Keep the workspaces a file the user chooses holds, and show the first of them.
     **/
    void importWorkspace();

    /**
     * The window changed size, so the layout divides a different area between the views.
     **/
    void handleResize(const v3d::event::kind::WindowResize& event);

 protected:
    /**
     * Build the document, the views, the tools, the renderer and the ui from the config the
     * engine read.
     * @return whether the editor can run
     **/
    bool start() override;

    /**
     **/
    bool release() override;

    /**
     * A quit with unsaved changes asks first, and quits once the user has saved or discarded
     * them. A request that arrives while the editor is already asking is held, and changes
     * nothing.
     * @return whether to quit now
     **/
    bool quitRequested() override;

 private:
    /**
     * Read the ui tree and find the menu bar and the toolbars in it.
     * @return whether there is a ui to draw
     **/
    bool buildUi();

    /**
     * Mark the menu items and toolbar buttons whose commands describe a state the editor
     * holds, so that what a check item and a toggle button show is read from the editor
     * rather than remembered by the ui.
     **/
    void syncUi();

    /**
     * Name the project in the window caption, and mark it when it has changes that are not
     * saved. The caption is only written when that changes.
     **/
    void syncCaption();

    /**
     * Register a handler for every command the editor supports, from the constructor. A
     * menu item naming a command that is not registered here logs a warning when it is
     * invoked. Each handler reads the editor's state back into the ui after it runs.
     **/
    void registerCommands();

    /**
     * Put the command search up over every command on the editor's menu bar and an area's
     * header, each with its shortcut.
     **/
    void openSearch();

    /**
     * Take the command search down and run the command chosen in it.
     **/
    void runSearch();

    /**
     * Log what an operation said, and show it as a notice.
     **/
    void report(const Outcome& outcome);

    /**
     * Log something the user asked for that could not be done, and show it as a notice.
     **/
    void refuse(const std::string& message);

    /**
     * @return what the status bar says this frame
     **/
    Status status() const;

    /**
     * Run an operation on the selected parts, when the select mask selects the parts it works
     * on. Under any other mask it says so and does nothing.
     **/
    void masked(SelectMask mask, const std::string& name, const std::function<Outcome()>& operation);

    /**
     * Run something that needs a view's camera, with the active view's. Without an active
     * view it says why nothing happened.
     * @param what what needs it, for the log
     **/
    void atView(const std::string& what, const std::function<void(const v3d::type::camera::Profile&)>& run);

    /**
     * The camera a render sees through: the one selected scene camera, or the active view.
     **/
    v3d::type::camera::Profile shot() const;

    /**
     * Choose a project to read over the document, write the document to its project, or choose
     * a name to write it under.
     **/
    void openProject();
    void saveProjectAs();

    /**
     * Choose a file to import into the document, of the extensions one format is read from.
     **/
    void importScene(const std::vector<std::string>& extensions);

    /**
     * Run something that replaces the document, asking first when it has unsaved changes.
     * Saving and then continuing, or discarding them and continuing, both run it. A save that
     * fails stops it, and so does cancelling.
     **/
    void askBeforeDiscarding(const std::function<void()>& proceed);

    /**
     * Offer to recover the project's autosave, when it holds changes no save kept.
     **/
    void offerRecovery();

    /**
     * Write the scene as RIB beside the executable, seen from the shot's camera.
     **/
    void exportProject();

    /**
     * Show a camera profile in an area's view, keeping the view's region.
     *
     * @param area an index into the layout's areas
     * @param view the view the area shows
     **/
    void look(std::size_t area, ViewPort* view, const std::string& profile);

    /**
     * Switch a view between perspective and orthographic.
     **/
    void toggleProjection(ViewPort* view);

    /**
     * Choose the transform tool's mode, or the select mask the select tool picks under and the
     * transform tool's handles move.
     **/
    void transformMode(const std::string& name);
    void selectMask(const std::string& name);

    /**
     * Hold or release a camera move for as long as its modifier is down.
     **/
    void cameraMode(const std::string& name, bool pressed);

    /**
     * Give each viewport editor of the layout a viewport, from the camera profile it names.
     * @return whether every view could be built
     **/
    bool buildViews();

    /**
     * Divide the window, less the ui's strips, between the views.
     **/
    void layoutViews(int width, int height);

    std::string path_;
    // the ui, the screen and the statistics overlay. The editor's menu is its own menu bar
    v3d::shell::Shell shell_;
    boost::shared_ptr<Document> document_;
    boost::shared_ptr<v3d::ui::shell::Keyboard> uiKeys_;
    boost::shared_ptr<v3d::ui::shell::FileChooser> chooser_;
    boost::shared_ptr<v3d::ui::shell::Prompt> prompt_;
    boost::shared_ptr<v3d::ui::component::menu::Bar> menu_;
    std::vector<boost::shared_ptr<v3d::ui::component::Toolbar>> toolbars_;
    // what the caption was last set to
    std::string caption_;
    boost::shared_ptr<v3d::engine::CameraProfiles> profiles_;
    boost::shared_ptr<AreaLayout> layout_;
    boost::shared_ptr<AreaViews> views_;
    // each area's header and tabs
    boost::shared_ptr<AreaChrome> chrome_;
    // what commands said, shown at the foot of the active area
    boost::shared_ptr<Notices> notices_;
    boost::shared_ptr<CommandSearch> search_;
    boost::shared_ptr<Workspaces> workspaces_;
    // the name of the workspace shown
    std::string workspace_;

    boost::shared_ptr<CameraControlTool> cameraTool_;
    boost::shared_ptr<SelectTool> selectTool_;
    boost::shared_ptr<TransformTool> transformTool_;
    boost::shared_ptr<ToolRouter> tools_;
    boost::shared_ptr<Pointer> pointer_;
    boost::shared_ptr<Playback> playback_;
    boost::shared_ptr<RenderWindow> renderWindow_;
    boost::shared_ptr<Renderer> renderer_;

    // names registered a second time, whose handlers would never run. The logger does not
    // exist while the constructor registers, so start() reports them
    std::vector<std::string> refused_;
    // the dispatcher holds delegates to this object. These disconnect them when it is destroyed
    entt::scoped_connection motion_;
    entt::scoped_connection button_;
    entt::scoped_connection resize_;
    // how many clicks in succession the last left press was, which view::drag hands the pointer
    unsigned int clicks_ = 1;
    // the ui scale the layout's metrics were last set for
    float metricsScale_ = 0.0F;
};

};  // namespace v3d::editor
