/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <api/config/CameraProfiles.h>
#include <api/engine/Engine.h>
#include <api/event/Event.h>
#include <api/event/kind/MouseMotion.h>
#include <api/event/kind/WindowResize.h>
#include <api/ui/input/Cursor.h>
#include <api/ui/shell/FileChooser.h>
#include <api/ui/shell/Keyboard.h>
#include <api/ui/shell/Prompt.h>
#include <api/ui/Engine.h>
#include <api/ui/component/Toolbar.h>
#include <api/ui/component/menu/Bar.h>
#include <vertical3d/src/command/CommandStack.h>
#include <vertical3d/src/command/TopologyCommand.h>
#include <vertical3d/src/scene/Project.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/tool/CameraControlTool.h>
#include <vertical3d/src/tool/SelectMask.h>
#include <vertical3d/src/tool/SelectTool.h>
#include <vertical3d/src/tool/TransformTool.h>
#include <vertical3d/src/view/ViewLayout.h>
#include <vertical3d/src/view/ViewPort.h>

#include <functional>
#include <string>
#include <vector>

#include <boost/filesystem/path.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::editor {

class Renderer;

/**
 * The editor application.
 *
 * It owns the camera profile table and the view layout, the viewports they produce
 * between them, and the tools that drive their cameras.
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
     * Only the keyboard goes this way. The ui's share of the mouse is decided in drag(),
     * from the mapped view::drag command: drag() offers each press to the ui first, and
     * passes a press the ui did not take to the tools.
     **/
    bool onEvent(const SDL_Event& event) override;

    /**
     * The cursor moved. A drag drives the camera of the view under the cursor, so this
     * chooses the active view.
     **/
    void handleMotion(const v3d::event::kind::MouseMotion& event);

    /**
     * The window changed size, so the layout divides a different area between the views.
     **/
    void handleResize(const v3d::event::kind::WindowResize& event);

 protected:
    /**
     * Build the scene, the views, the tools, the renderer and the ui from the config the
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
     * Offer the cursor to the ui before the tools see it. The ui library decides which of
     * its parts is offered the cursor first.
     * @return whether the ui took it
     **/
    bool uiMotion(const glm::vec2& cursor);

    /**
     * Offer a press to the ui before the tools see it.
     * @return whether the ui took it
     **/
    bool uiPress(const glm::vec2& cursor);

    /**
     * Register a handler for every command the editor supports, from the constructor. A
     * menu item naming a command that is not registered here logs a warning when it is
     * invoked. Each handler reads the editor's state back into the ui after it runs.
     **/
    void registerCommands();

    /**
     * Put one of the polygon primitives into the scene, at the origin.
     * @param name which primitive, as the create commands name it
     **/
    void createPoly(const std::string& name);

    /**
     * Split the selected edges at their middles. It works under the edge mask only, and logs
     * why when it does nothing.
     **/
    void splitSelectedEdges();

    /**
     * Split a face between the two selected vertices. It works under the vertex mask only,
     * and logs why when it does nothing.
     **/
    void splitSelectedFace();

    /** An operation on the selected faces, such as extrudeFaces() in Modelling.h. **/
    using FaceOperation = boost::shared_ptr<TopologyCommand> (*)(const Scene& scene);

    /**
     * Run an operation on the selected faces. It works under the face mask only, and logs why
     * when it does nothing.
     * @param name what the operation is called, for the log
     **/
    void changeSelectedFaces(const std::string& name, FaceOperation operation);

    /**
     * Step the history one command in either direction and say so.
     * @param name either "undo" or "redo"
     **/
    void history(const std::string& name);

    /**
     * Choose a project to read over the scene, write the scene to the project it came from,
     * or choose a name to write it under. The project starts as project.json beside the
     * executable, and is whatever was last opened or saved as after that.
     **/
    void openProject();
    bool saveProject();
    void saveProjectAs();

    /**
     * Run something that replaces the scene, asking first when the project has unsaved
     * changes. Saving and then continuing, or discarding them and continuing, both run it.
     * A save that fails stops it, and so does cancelling.
     * @param proceed what replaces the scene
     **/
    void askBeforeDiscarding(const std::function<void()>& proceed);

    /**
     * Write the scene out as RIB for the offline renderer.
     *
     * It writes the topology and a placement per mesh, seen from the active view's camera.
     * Nothing reads the RIB back. The editor saves and opens its own project format.
     **/
    void exportProject();

    /**
     * @return where the RIB export goes, beside the executable
     **/
    std::string exportPath() const;

    /**
     * Turn one of a view's visibility flags on or off.
     * @param filter which flag
     **/
    void toggleShow(ViewPort::VisibleFilter filter);

    /**
     * The primary mouse button went down or up. It drives the camera, transform and select
     * tools, and this decides which one takes it.
     **/
    void drag(bool pressed);

    /**
     * Choose the transform tool's mode.
     * @param name the mode, as TransformTool names it
     **/
    void transformMode(const std::string& name);

    /**
     * Choose the select mask, for the select tool that picks under it and for the transform
     * tool, whose handles move what the mask selects.
     * @param name the mask, as SelectMask names it
     **/
    void selectMask(const std::string& name);

    /**
     * Hold or release a camera move for as long as its modifier is down.
     * @param name the move, as CameraControlTool names it
     * @param pressed whether the modifier went down or came up
     **/
    void cameraMode(const std::string& name, bool pressed);

    /**
     * Build one viewport per leaf of the layout, with the profile it names.
     * @return whether every view could be built
     **/
    bool buildViews();

    /**
     * Divide the window between the views and tell each one its region.
     **/
    void layoutViews(int width, int height);

    std::string path_;
    boost::shared_ptr<Scene> scene_;
    boost::shared_ptr<v3d::ui::Engine> vgui_;
    boost::shared_ptr<v3d::ui::input::Cursor> uiCursor_;
    boost::shared_ptr<v3d::ui::shell::Keyboard> uiKeys_;
    boost::shared_ptr<v3d::ui::shell::FileChooser> chooser_;
    boost::shared_ptr<v3d::ui::shell::Prompt> prompt_;
    boost::shared_ptr<v3d::ui::component::menu::Bar> menu_;
    std::vector<boost::shared_ptr<v3d::ui::component::Toolbar>> toolbars_;
    boost::shared_ptr<Project> project_;
    // the document Save writes to
    boost::filesystem::path projectPath_;
    boost::shared_ptr<CommandStack> history_;
    // what the caption was last set to
    std::string caption_;
    boost::shared_ptr<v3d::config::CameraProfiles> profiles_;
    boost::shared_ptr<ViewLayout> layout_;
    std::vector<boost::shared_ptr<ViewPort>> views_;
    boost::shared_ptr<ViewPort> activeView_;

    boost::shared_ptr<CameraControlTool> cameraTool_;
    boost::shared_ptr<SelectTool> selectTool_;
    boost::shared_ptr<TransformTool> transformTool_;
    boost::shared_ptr<Renderer> renderer_;

    glm::vec2 cursor_;
    // whether the ui took the press, so that the release that ends it does not reach
    // the tools that never saw the press
    bool uiGrab_;
    // names registered a second time, whose handlers would never run. The logger does not
    // exist while the constructor registers, so start() reports them
    std::vector<std::string> refused_;
    // the dispatcher holds delegates to this object. These disconnect them when it is destroyed
    entt::scoped_connection motion_;
    entt::scoped_connection resize_;
};

};  // namespace v3d::editor
