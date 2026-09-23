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
#include <api/ui/shell/Keyboard.h>
#include <api/ui/Engine.h>
#include <api/ui/component/Toolbar.h>
#include <api/ui/component/menu/MenuBar.h>
#include <vertical3d/src/command/CommandDirectory.h>
#include <vertical3d/src/command/CommandStack.h>
#include <vertical3d/src/scene/Project.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/tool/CameraControlTool.h>
#include <vertical3d/src/tool/SelectMask.h>
#include <vertical3d/src/tool/SelectTool.h>
#include <vertical3d/src/tool/TransformTool.h>
#include <vertical3d/src/view/ViewLayout.h>
#include <vertical3d/src/view/ViewPort.h>

#include <string>
#include <vector>

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
     * Bring up the window, read the config, and build the views out of it.
     * @return whether the editor can run
     **/
    bool initialize();

    /**
     **/
    bool render() override;

    /**
     **/
    bool shutdown() override;

    /**
     * Offer every event to the ui before the bindings map it, per ADR-0043.
     *
     * Only the keyboard goes this way. A press has to interleave with the camera and the
     * transform tools - drag() offers the ui the press and drives a camera with the one it
     * did not take - so the cursor is routed from a mapped command.
     **/
    bool onEvent(const SDL_Event& event) override;

    /**
     * A mapped event. Its identity is a command name, so this is a lookup in the
     * directory and nothing else.
     **/
    void handleEvent(const v3d::event::Event& event);

    /**
     * The cursor moved. Which view it is over is what decides which camera a drag
     * drives, so this is where the active view is chosen.
     **/
    void handleMotion(const v3d::event::kind::MouseMotion& event);

    /**
     * The window changed size, so the layout divides a different area between the views.
     **/
    void handleResize(const v3d::event::kind::WindowResize& event);

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
     * Offer the cursor to the ui before the tools see it. Which part of the ui is offered
     * it first is the library's, per ADR-0038.
     * @return whether the ui took it
     **/
    bool uiMotion(const glm::vec2& cursor);

    /**
     * Offer a press to the ui before the tools see it.
     * @return whether the ui took it
     **/
    bool uiPress(const glm::vec2& cursor);

    /**
     * Register a handler for every command the editor answers to. What is not in here
     * is what the editor cannot do, which is how an untranslated menu item reports
     * itself.
     **/
    void registerCommands();

    /**
     * Put one of the polygon primitives into the scene, at the origin.
     * @param name which primitive, as the create commands name it
     **/
    void createPoly(const std::string& name);

    /**
     * Step the history one command in either direction and say so.
     * @param name either "undo" or "redo"
     **/
    void history(const std::string& name);

    /**
     * Read the project over the scene, or write the scene out as one.
     *
     * There is no file chooser in the tree, so both work on one document at a fixed
     * path - see ADR-0018.
     **/
    void openProject();
    void saveProject();

    /**
     * Write the scene out as RIB for the offline renderers, per ADR-0023.
     *
     * One way: topology and a placement per mesh, from the active view's camera. The
     * project format stays the editor's own and nothing reads this back.
     **/
    void exportProject();

    /**
     * @return where the one document lives, beside the executable
     **/
    std::string projectPath() const;

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
     * The primary mouse button, which drives three tools - see registerCommands().
     **/
    void drag(bool pressed);

    /**
     * Choose the transform tool's mode and tell the renderer which handles to draw.
     * @param name the mode, as TransformTool names it
     **/
    void transformMode(const std::string& name);

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
    boost::shared_ptr<v3d::ui::component::MenuBar> menu_;
    std::vector<boost::shared_ptr<v3d::ui::component::Toolbar>> toolbars_;
    boost::shared_ptr<Project> project_;
    CommandDirectory directory_;
    boost::shared_ptr<CommandStack> commands_;
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
};

};  // namespace v3d::editor
