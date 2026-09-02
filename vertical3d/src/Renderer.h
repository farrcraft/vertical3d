/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vector>

#include "Scene.h"
#include "ViewPort.h"

#include "../../api/asset/Manager.h"
#include "../../api/log/Logger.h"
#include "../../api/render/realtime/Engine3D.h"
#include "../../api/render/realtime/LineCanvas.h"
#include "../../api/render/realtime/Window.h"

#include <boost/shared_ptr.hpp>
#include <entt/entt.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

    /**
     * The editor's frame: one pass per viewport, over one device.
     *
     * Four views of one scene is four passes with four cameras, per ADR-0003. Each pass
     * carries its viewport's region as its scissor and its camera at set 0, and clears its
     * own region - so the split is a property of the frame rather than of the window.
     *
     * Every pass depth tests, because the wireframe a modeller draws has to be occluded by
     * what is in front of it. They all share one depth buffer, which each clears within its
     * own region.
     */
    class Renderer final {
     public:
        /**
         * @throw std::runtime_error if the device or the line pipelines cannot be built
         **/
        Renderer(const boost::shared_ptr<v3d::render::realtime::Window>& window,
            const boost::shared_ptr<v3d::log::Logger>& logger,
            const boost::shared_ptr<v3d::asset::Manager>& assetManager, entt::registry* registry);

        /**
         **/
        ~Renderer();

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        /**
         * The views to draw, in the order their passes are recorded.
         **/
        void views(const std::vector<boost::shared_ptr<ViewPort>>& views);

        /**
         * What every view draws. One scene, four passes.
         **/
        void scene(const boost::shared_ptr<Scene>& scene);

        /**
         * Draw one frame - a pass per view.
         **/
        void draw();

        /**
         * Wait for everything in flight, before the window the device draws to goes away.
         **/
        void shutdown();

     private:
        boost::shared_ptr<v3d::log::Logger> logger_;

        // first, so that everything holding a device handle below is destroyed before the
        // context that owns the device is
        v3d::render::realtime::Engine3D engine_;

        boost::shared_ptr<Scene> scene_;
        std::vector<boost::shared_ptr<ViewPort>> views_;
        // one canvas per view rather than one shared: a canvas becomes a single draw item,
        // and each is filled before any of them is submitted
        std::vector<v3d::render::realtime::LineCanvas> canvases_;
        glm::vec4 background_;
    };

};  // namespace v3d::editor
