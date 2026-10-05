/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Manager.h>
#include <api/log/Logger.h>
#include <api/render/realtime/Canvas.h>
#include <api/render/realtime/Engine3D.h>
#include <api/render/realtime/vulkan/memory/DeviceBuffer.h>
#include <api/render/realtime/vulkan/pipeline/DescriptorPool.h>
#include <api/ui/Engine.h>
#include <api/ui/Immediate.h>
#include <api/ui/shell/Screen.h>
#include <api/ui/shell/StatisticsOverlay.h>

#include <vulkan/vulkan.h>

#include <cstddef>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <entt/entt.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

class Scene;
class ChunkMeshPool;
class MeshBuilder;

/**
 * The terrain, and the text drawn over it.
 *
 * Two passes, because the two need opposite settings. The terrain is a depth tested, sorted
 * scene of one draw item per chunk through a pipeline of its own. The overlay and the ui
 * are painter ordered quads, drawn on top of it through the batched quad pipeline. Each pass
 * carries its own settings, so neither has to know about the other.
 */
class Renderer {
 public:
    /**
     * @throw std::runtime_error if the pipeline or its uniforms cannot be built
     **/
    Renderer(const boost::shared_ptr<Scene> & scene, const boost::shared_ptr<v3d::render::realtime::Window>& window,
        const boost::shared_ptr<v3d::log::Logger> & logger, const boost::shared_ptr<v3d::asset::Manager>& assetManager);

    /**
     **/
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    /**
     * Draw the frame
     */
    /**
     * @param statistics what the loop measured about its own pacing, which the debug
     *        window reads. The app passes it in because api/ui cannot depend on api/engine
     * @param tools what the cursor did, for the immediate layer - Controller::tools()
     **/
    /**
     * @return how long the device spent on each pass, by its name
     **/
    const std::vector<v3d::render::realtime::vulkan::frame::Timings::Timing>& timings() const;

    void draw(const v3d::ui::shell::StatisticsOverlay::Sample& statistics,
        const v3d::ui::Immediate::Input& tools);
    /**
     * Resize the frame
     */
    void resize(int width, int height);

    void tick(unsigned int delta);

    void debug(bool status);

    /**
     * The ui whose containers are drawn over the terrain, or null to draw none.
     **/
    void ui(const boost::shared_ptr<v3d::ui::Engine>& engine);

    /**
     * Wait for everything in flight, before the window the device draws to goes away.
     **/
    void shutdown();

 private:
    /**
     * The layout of set 1 - the light and the block palette, which every chunk draws with.
     **/
    void createLayout();

    /**
     * Fill the palette and upload it, once. Nothing in the world changes a material.
     **/
    void createUniforms();

    /**
     * Compile the terrain pipeline against the pass it draws into - a colour attachment and
     * a depth one, which dynamic rendering needs named at compile time.
     **/
    void createPipeline();

    /**
     * One draw item per meshed chunk in the camera's view, submitted to the terrain pass.
     **/
    void drawTerrain(v3d::render::realtime::Pass* pass);

    /**
     * The F3 readout - the build, what the loop measured, where the player is standing, and
     * how many of the meshed chunks were drawn.
     **/
    void drawDebug(const v3d::ui::shell::StatisticsOverlay::Sample& statistics,
        const v3d::ui::Immediate::Input& tools);

    boost::shared_ptr<Scene> scene_;
    boost::shared_ptr<v3d::log::Logger> logger_;

    // first, so that everything holding a device handle below is destroyed before the
    // context that owns the device is
    v3d::render::realtime::Engine3D engine_;

    boost::shared_ptr<v3d::render::realtime::DeviceContext> context_;
    boost::shared_ptr<v3d::render::realtime::vulkan::pipeline::DescriptorPool> scenePool_;
    boost::shared_ptr<v3d::render::realtime::vulkan::memory::DeviceBuffer> uniforms_;
    v3d::render::realtime::PipelineHandle pipeline_;
    v3d::render::realtime::MaterialHandle material_;

    boost::shared_ptr<ChunkMeshPool> meshes_;
    boost::shared_ptr<MeshBuilder> builder_;
    std::size_t drawnChunks_;   /**< how many chunks the last frame drew **/
    std::size_t meshedChunks_;  /**< how many it could have, being meshed **/

    bool debug_;


    boost::shared_ptr<v3d::ui::Engine> ui_;
    // built after the engine is initialized, because its atlas is uploaded through it
    boost::shared_ptr<v3d::ui::shell::Screen> screen_;
};
