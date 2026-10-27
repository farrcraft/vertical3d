/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <api/engine/CameraProfiles.h>
#include <api/engine/Engine.h>
#include <api/event/kind/MouseMotion.h>
#include <api/event/kind/WindowResize.h>
#include <api/shell/Shell.h>
#include <api/type/camera/Profile.h>
#include <api/ui/component/Toolbar.h>
#include <api/ui/component/menu/Bar.h>
#include <api/ui/shell/FileChooser.h>
#include <api/ui/shell/Keyboard.h>
#include <api/ui/shell/Prompt.h>
#include <vertical3d/src/control/Playback.h>
#include <vertical3d/src/control/Pointer.h>
#include <vertical3d/src/control/RenderWindow.h>
#include <vertical3d/src/document/Document.h>
#include <vertical3d/src/selection/SelectMask.h>
#include <vertical3d/src/tool/CameraControlTool.h>
#include <vertical3d/src/tool/SelectTool.h>
#include <vertical3d/src/tool/ToolRouter.h>
#include <vertical3d/src/tool/TransformTool.h>
#include <vertical3d/src/view/ViewLayout.h>
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
     * Offer every event to the ui keyboard before the bindings map it.
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
     * view.
     **/
    void handleMotion(const v3d::event::kind::MouseMotion& event);

    /**
     * The window changed size, so the layout divides a different area between the views.
     **/
    void handleResize(const v3d::event::kind::WindowResize& event);

    /** A command that shows a fixed view, and the camera profile that view is. **/
    struct FixedView {
        std::string command;
        std::string profile;
    };

    /**
     * The fixed views the View menu offers. Each command shows its profile in the active view.
     **/
    static const std::vector<FixedView>& fixedViews();

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
     * Log what an operation said.
     **/
    void report(const Outcome& outcome);

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
     * Show a camera profile in the active view, keeping the view's region, or switch the
     * active view between perspective and orthographic.
     **/
    void look(const std::string& profile);
    void toggleProjection();

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
     * Build one viewport per leaf of the layout, with the profile it names.
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
    boost::shared_ptr<ViewLayout> layout_;
    std::vector<boost::shared_ptr<ViewPort>> views_;

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
    entt::scoped_connection resize_;
};

};  // namespace v3d::editor
