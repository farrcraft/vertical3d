/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <map>
#include <string>

#include "voxel/Voxel.h"
#include "voxel/MeshBuilder.h"

#include "../../api/log/Logger.h"
#include "../../api/ui/Window.h"
#include "../../api/gl/Program.h"

#include <boost/shared_ptr.hpp>

class Scene;
class AssetLoader;
class DebugOverlay;
class ChunkBufferPool;

/**
 * Main engine renderer
 */
class Renderer {
 public:
    /**
     * Default Constructor
     */
    Renderer(const boost::shared_ptr<Scene> & scene, const boost::shared_ptr<AssetLoader> & loader, const boost::shared_ptr<v3d::log::Logger> & logger);

    /**
     * Draw the frame
     */
    void draw(boost::shared_ptr<v3d::ui::Window> window);
    /**
     * Resize the frame
     */
    void resize(int width, int height);

    void tick(unsigned int delta);

    void debug(bool status);

 private:
        std::map<std::string, boost::shared_ptr<v3d::gl::Program> > programs_;
        boost::shared_ptr<Scene> scene_;
        bool debug_;
        boost::shared_ptr<DebugOverlay> debugOverlay_;
        boost::shared_ptr<ChunkBufferPool> pool_;
        MeshBuilder builder_;
        boost::shared_ptr<v3d::log::Logger> logger_;
};
