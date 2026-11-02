/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <api/engine/Engine.h>
#include <api/event/kind/MouseButton.h>
#include <api/event/kind/MouseMotion.h>
#include <api/event/kind/WindowResize.h>
#include <api/shell/Shell.h>
#include <api/ui/component/Toolbar.h>
#include <api/ui/component/menu/Bar.h>
#include <api/ui/shell/Keyboard.h>
#include <vertical3d/src/app/AreaCommands.h>
#include <vertical3d/src/app/CommandTable.h>
#include <vertical3d/src/app/Context.h>
#include <vertical3d/src/app/EditCommands.h>
#include <vertical3d/src/app/PlaybackCommands.h>
#include <vertical3d/src/app/ProjectFiles.h>
#include <vertical3d/src/app/RenderCommands.h>
#include <vertical3d/src/app/ToolCommands.h>
#include <vertical3d/src/app/ViewCommands.h>
#include <vertical3d/src/app/WorkspaceSwitcher.h>
#include <vertical3d/src/control/chrome/Area.h>
#include <vertical3d/src/control/CommandSearch.h>
#include <vertical3d/src/control/GraphEditor.h>
#include <vertical3d/src/control/Outliner.h>
#include <vertical3d/src/control/PropertiesEditor.h>
#include <vertical3d/src/control/RenderSettingsEditor.h>
#include <vertical3d/src/control/RenderView.h>
#include <vertical3d/src/control/chrome/StatusBar.h>
#include <vertical3d/src/control/chrome/TopBar.h>
#include <vertical3d/src/tool/Router.h>

#include <string>
#include <string_view>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <entt/entt.hpp>

namespace v3d::editor {

class Renderer;

/**
 * The editor application.
 *
 * It builds the document, the views, the tools and the ui at start, and joins them. Each part
 * of the editor registers its own commands: the document's edits, the tools, the views and the
 * areas, the workspaces, the project's files, playback and the Render menu. They act on the
 * context this fills in at start. What is left here is building the parts, the ui that shows
 * them, the routing of input, and the command search.
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
     * What the ui config calls the container the editor's own components are in, and the
     * menu bar within it. Every other component of that container is a toolbar.
     **/
    static constexpr const char* uiContainer = "editor";
    static constexpr const char* menuBar = "menu-bar";

    /**
     * The first container the ui config lists after the editor's own. The top bar goes before it,
     * so it is drawn over the menu bar and under every dialog.
     **/
    static constexpr const char* firstDialog = "chooser";

#ifdef _WIN32
    static constexpr const char* MOYA = "moya.exe";
#else
    static constexpr const char* MOYA = "moya";
#endif

    /**
     * The editor's shell. It has no game menu, because the editor's menu is its menu bar.
     **/
    static v3d::shell::Shell::Options shellOptions();

    /**
     * Register every part's commands, and the controller's own: the command search and the
     * drag of the primary button. A menu item naming a command that is not registered logs a
     * warning when it is invoked.
     **/
    void registerCommands();

    /**
     * Make the document and the playback of its timeline, and read the camera profiles.
     * @return whether there are cameras for the views
     **/
    bool buildDocument();

    /**
     * Give each viewport editor of the layout a viewport, from the camera profile it names.
     * @return whether every view could be built
     **/
    bool buildViews();

    /**
     * Make the camera, select and transform tools, and the router one button drives them
     * through. With a camera move held or latched, the drag drives a camera. Otherwise a
     * handle of the selection under the cursor takes the press, and a press no handle took
     * picks.
     **/
    void buildTools();

    /**
     * Make the renderer, and give it the icons, the views, the scene, the tools and the notices
     * it draws.
     **/
    void buildRenderer();

    /**
     * Read the ui tree, make the controllers that drive its parts, and find the menu bar and
     * the toolbars in it.
     * @return whether there is a ui to draw
     **/
    bool buildUi();

    /**
     * Make the pointer, which routes the mouse to the ui, the tabs, a view's gizmo or the tools.
     **/
    void buildPointer(const boost::shared_ptr<v3d::ui::Interface>& vgui);

    /**
     * Find the menu bar and the toolbars in the editor's own container.
     * @return whether the container and its menu bar are there
     **/
    bool findBars(const boost::shared_ptr<v3d::ui::Interface>& vgui);

    /**
     * Log any command registered twice, and how many commands there are.
     **/
    void reportCommands() const;

    /**
     * Mark the menu items and toolbar buttons whose commands describe a state the editor
     * holds. A check item and a toggle button then show what the editor holds, rather than
     * a state the ui remembers.
     **/
    void syncUi();

    /**
     * Read the editor's state back into the ui and the caption after a command runs.
     **/
    void commandRan();

    /**
     * Name the project in the window caption, and mark it when it has changes that are not
     * saved. The caption is only written when that changes.
     **/
    void syncCaption();

    /**
     * Show the pointer as a resize cursor over a divider that is lit, and as an arrow elsewhere.
     **/
    void syncPointerShape();

    /**
     * Give the views their regions again after a change to the areas.
     **/
    void areasChanged();

    /**
     * Draw the panel of each area that shows an editor other than a view, under the header,
     * through the immediate layer. The layer has the mouse only while the pointer gives the
     * panels the cursor.
     **/
    void drawPanels(v3d::ui::Immediate* layer, v3d::render::realtime::canvas::Canvas* canvas);

    /**
     * @return whether any area shows an editor of a kind
     **/
    bool shows(std::string_view kind) const;

    /**
     * @return what the top bar shows: the workspaces, the one shown, and the project's name and
     *         whether it has unsaved changes
     **/
    control::chrome::TopBar::State topState() const;

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
     * @return what the status bar says this frame
     **/
    control::chrome::Status status() const;

    /**
     * Divide the window, less the ui's strips, between the views.
     **/
    void layoutViews(int width, int height);

    // what the commands act on, which start() fills in
    app::Context context_;
    // the ui, the screen and the statistics overlay. The editor's menu is its own menu bar
    v3d::shell::Shell shell_;
    app::CommandTable table_;
    app::EditCommands edits_;
    app::ToolCommands toolCommands_;
    app::ViewCommands viewCommands_;
    app::AreaCommands areaCommands_;
    app::WorkspaceSwitcher workspaces_;
    app::ProjectFiles files_;
    app::PlaybackCommands playbackCommands_;
    app::RenderCommands renderCommands_;

    boost::shared_ptr<v3d::ui::shell::Keyboard> uiKeys_;
    boost::shared_ptr<v3d::ui::component::menu::Bar> menu_;
    std::vector<boost::shared_ptr<v3d::ui::component::Toolbar>> toolbars_;
    // what the caption was last set to
    std::string caption_;
    // each area's header and tabs
    boost::shared_ptr<control::chrome::Area> chrome_;
    boost::shared_ptr<control::CommandSearch> search_;
    control::Outliner outliner_;
    boost::shared_ptr<control::PropertiesEditor> properties_;
    // every graph editor, which share the chosen key
    boost::shared_ptr<control::GraphEditor> graph_;
    boost::shared_ptr<control::RenderSettingsEditor> renderSettings_;
    // what a render view's buttons asked for in the last frame drawn
    control::RenderView::Asked asked_ = control::RenderView::Asked::Nothing;
    // the workspaces, the project and the search, beside the menus
    boost::shared_ptr<control::chrome::TopBar> topBar_;
    boost::shared_ptr<tool::Router> tools_;
    boost::shared_ptr<Renderer> renderer_;

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
