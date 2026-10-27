/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Manager.h>
#include <api/log/Logger.h>
#include <api/render/realtime/LitScene.h>
#include <api/render/realtime/MeshRegistry.h>
#include <api/render/realtime/WindowRenderer.h>
#include <api/render/realtime/lighting/Settings.h>
#include <api/render/realtime/lighting/Shadow.h>
#include <api/shell/Shell.h>
#include <api/type/camera/Camera.h>
#include <api/type/camera/Isometric.h>
#include <api/ui/shell/Screen.h>
#include <api/window/Window.h>
#include <demos/diorama/src/Layout.h>

#include <cstdint>
#include <optional>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <entt/entt.hpp>
#include <glm/vec3.hpp>

namespace diorama {

/**
 * Everything the diorama draws: its models through a lit scene with a shadow map, graded into
 * the window's colour pass, and the shell's menu and statistics over them.
 *
 * The lit scene is built at the first frame, when the window's size is known, and replaced in
 * size whenever the window changes. The shadow map is fitted once, around the casters
 * place() stood up, because none of them moves.
 **/
class Renderer final {
 public:
    /**
     * @param shell the app's shell, which builds its screen over this renderer's engine. It
     *        outlives the renderer
     * @throw std::runtime_error if the device or the swapchain cannot be built
     **/
    Renderer(const boost::shared_ptr<v3d::window::Window>& window, const boost::shared_ptr<v3d::log::Logger>& logger,
        const boost::shared_ptr<v3d::asset::Manager>& assets, v3d::shell::Shell* shell);

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    /**
     * Load each placement's model and stand an entity for it in the registry, then fit the
     * shadow map around the ones that cast. A model several placements name is loaded once.
     *
     * @return false when a model cannot be loaded, which is logged
     **/
    bool place(entt::registry* registry, const std::vector<Placement>& placements);

    /**
     * Build the frame and present it.
     *
     * @param orbit where the camera is, which is written onto the camera this draws through
     * @param light the direction towards the sun
     * @param alpha engine::Engine::alpha()
     **/
    void draw(const v3d::type::camera::Isometric& orbit, const glm::vec3& light, float alpha, const entt::registry& registry);

    /**
     * @return the camera the last frame was drawn through, which the listener follows
     **/
    const v3d::type::camera::Camera& camera() const noexcept;

    /**
     * Release the scene and the models, and wait for everything in flight, before the window
     * the device draws to goes away.
     **/
    void shutdown();

 private:
    /**
     * Build the lit scene at a size, or replace its target with one of a new size, and give the
     * camera the window's aspect.
     **/
    void resize(uint32_t width, uint32_t height);

    boost::shared_ptr<v3d::log::Logger> logger_;
    v3d::shell::Shell* shell_;
    v3d::render::realtime::WindowRenderer engine_;
    boost::shared_ptr<v3d::render::realtime::MeshRegistry> meshes_;
    boost::shared_ptr<v3d::render::realtime::LitScene> scene_;
    v3d::render::realtime::lighting::Settings settings_;
    std::optional<v3d::render::realtime::lighting::ShadowBounds> bounds_;
    v3d::type::camera::Camera camera_;
    // the shell's screen, built over engine_
    v3d::ui::shell::Screen* screen_ = nullptr;
};

};  // namespace diorama
