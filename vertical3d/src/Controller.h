/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <string>
#include <vector>

#include "CameraControlTool.h"
#include "CameraProfiles.h"
#include "CommandDirectory.h"
#include "CommandStack.h"
#include "Project.h"
#include "Scene.h"
#include "SelectMask.h"
#include "SelectTool.h"
#include "TransformTool.h"
#include "ViewLayout.h"
#include "ViewPort.h"

#include "../../api/engine/Engine.h"
#include "../../api/event/Event.h"
#include "../../api/event/MouseMotion.h"
#include "../../api/event/WindowResize.h"

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
         * A mapped event. Its identity is a command name, so this is a lookup in the
         * directory and nothing else.
         **/
        void handleEvent(const v3d::event::Event& event);

        /**
         * The cursor moved. Which view it is over is what decides which camera a drag
         * drives, so this is where the active view is chosen.
         **/
        void handleMotion(const v3d::event::MouseMotion& event);

        /**
         * The window changed size, so the layout divides a different area between the views.
         **/
        void handleResize(const v3d::event::WindowResize& event);

     private:
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
         * @return where the one document lives, beside the executable
         **/
        std::string projectPath() const;

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
        boost::shared_ptr<Project> project_;
        CommandDirectory directory_;
        boost::shared_ptr<CommandStack> commands_;
        boost::shared_ptr<CameraProfiles> profiles_;
        boost::shared_ptr<ViewLayout> layout_;
        std::vector<boost::shared_ptr<ViewPort>> views_;
        boost::shared_ptr<ViewPort> activeView_;

        boost::shared_ptr<CameraControlTool> cameraTool_;
        boost::shared_ptr<SelectTool> selectTool_;
        boost::shared_ptr<TransformTool> transformTool_;
        boost::shared_ptr<Renderer> renderer_;

        glm::vec2 cursor_;
    };

};  // namespace v3d::editor
