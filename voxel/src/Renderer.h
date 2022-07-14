/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <map>
#include <string>

#include "voxel/Voxel.h"
#include "voxel/MeshBuilder.h"

#include "../../v3dlibs/hookah/Window.h"
#include "../../v3dlibs/gl/Program.h"

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
    Renderer(const boost::shared_ptr<Scene> & scene, const boost::shared_ptr<AssetLoader> & loader);

    /**
     * Draw the frame
     */
    void draw(Hookah::Window * window);
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
};
