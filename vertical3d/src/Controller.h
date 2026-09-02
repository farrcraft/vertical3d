/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <string>
#include <vector>

#include "CameraControlTool.h"
#include "CameraProfiles.h"
#include "Scene.h"
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
         * A mapped event - a camera mode modifier, a drag, or a command.
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
         * Put one of the polygon primitives into the scene, at the origin.
         * @param name which primitive, as the create binding names it
         * @return whether the name was one of them
         **/
        bool createPoly(const std::string& name);

        /**
         * Build one viewport per leaf of the layout, with the profile it names.
         * @return whether every view could be built
         **/
        bool buildViews();

        /**
         * Divide the window between the views and tell each one its region.
         **/
        void layoutViews(int width, int height);

        boost::shared_ptr<Scene> scene_;
        boost::shared_ptr<CameraProfiles> profiles_;
        boost::shared_ptr<ViewLayout> layout_;
        std::vector<boost::shared_ptr<ViewPort>> views_;
        boost::shared_ptr<ViewPort> activeView_;

        boost::shared_ptr<CameraControlTool> cameraTool_;
        boost::shared_ptr<Renderer> renderer_;

        glm::vec2 cursor_;
    };

};  // namespace v3d::editor
