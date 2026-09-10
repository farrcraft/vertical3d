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
#include <api/ui/paint/ComponentRenderer.h>
#include <api/ui/Engine.h>
#include <api/ui/Immediate.h>
#include <api/ui/shell/StatisticsOverlay.h>
#include <api/ui/paint/TextRenderer.h>

#include <vulkan/vulkan.h>

#include <string>

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
 * Two passes, because the two want opposite things from the frame: the terrain is a depth
 * tested, sorted scene of one draw item per chunk through a pipeline of its own, and the
 * overlay and the ui are painter ordered quads on the batched primitive of ADR-0005 drawn on
 * top of it. The pass is the unit of variation, per ADR-0003, so neither has to know about
 * the other.
 */
class Renderer {
 public:
    /**
     * @throw std::runtime_error if the pipeline or its uniforms cannot be built
     **/
    Renderer(const boost::shared_ptr<Scene> & scene, const boost::shared_ptr<v3d::render::realtime::Window>& window,
        const boost::shared_ptr<v3d::log::Logger> & logger, const boost::shared_ptr<v3d::asset::Manager>& assetManager, entt::registry* registry);

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
     *        window reads - the app hands it over because api/ui sits below api/engine
     **/
    void draw(const v3d::ui::shell::StatisticsOverlay::Sample& statistics);
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
     * One draw item per meshed chunk, submitted to the terrain pass.
     **/
    void drawTerrain(v3d::render::realtime::Pass* pass);

    /**
     * The F3 readout - the build, what the loop measured, and where the player is standing.
     **/
    void drawDebug(const v3d::ui::shell::StatisticsOverlay::Sample& statistics);

    boost::shared_ptr<Scene> scene_;
    boost::shared_ptr<v3d::log::Logger> logger_;

    // first, so that everything holding a device handle below is destroyed before the
    // context that owns the device is
    v3d::render::realtime::Engine3D engine_;

    boost::shared_ptr<v3d::render::realtime::Context3D> context_;
    VkDescriptorSetLayout sceneLayout_;
    VkDescriptorPool pool_;
    boost::shared_ptr<v3d::render::realtime::vulkan::memory::DeviceBuffer> uniforms_;
    v3d::render::realtime::PipelineHandle pipeline_;
    v3d::render::realtime::MaterialHandle material_;

    boost::shared_ptr<ChunkMeshPool> meshes_;
    boost::shared_ptr<MeshBuilder> builder_;

    bool debug_;

    v3d::render::realtime::Canvas canvas_;
    boost::shared_ptr<v3d::ui::paint::TextRenderer> text_;

    boost::shared_ptr<v3d::ui::Engine> ui_;
    boost::shared_ptr<v3d::ui::paint::ComponentRenderer> uiRenderer_;
    boost::shared_ptr<v3d::ui::Immediate> tools_;
};
