/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Manager.h>
#include <api/font/Paragraph.h>
#include <api/font/ShapingCache.h>
#include <api/log/Logger.h>
#include <api/render/realtime/LitScene.h>
#include <api/render/realtime/MeshRegistry.h>
#include <api/render/realtime/WindowRenderer.h>
#include <api/render/realtime/lighting/Settings.h>
#include <api/render/realtime/lighting/Shadow.h>
#include <api/render/realtime/renderer/World.h>
#include <api/shell/Shell.h>
#include <api/type/camera/Camera.h>
#include <api/type/camera/Isometric.h>
#include <api/ui/paint/CurveText.h>
#include <api/ui/shell/Screen.h>
#include <api/window/Window.h>
#include <demos/diorama/src/Layout.h>

#include <cstdint>
#include <optional>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <entt/entt.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace diorama {

/**
 * Everything the diorama draws: its models through a lit scene with a shadow map, a sign standing
 * over the well, graded into the window's colour pass, and the shell's menu and statistics over
 * them.
 *
 * The sign's words are drawn from their curves, standing in the world above the well and turned
 * to face the camera, so they are sharp from every side and at every zoom. A house in front of
 * the sign hides it.
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
    /** What the lit pass clears to, a pale blue sky, in linear light. **/
    static constexpr glm::vec4 SKY{0.45F, 0.62F, 0.85F, 1.0F};

    /** The width of the shadow map, in texels. **/
    static constexpr uint32_t SHADOW_TEXELS = 2048;

    /**
     * How far past the casters the shadow map reaches, for the height of a tree and the length of
     * its shadow, in world units.
     **/
    static constexpr float SHADOW_MARGIN = 3.0F;

    /**
     * The camera's near and far planes. The orbit's eye is 30 units from the target, so the far
     * plane takes in the whole ground from any azimuth.
     **/
    static constexpr glm::vec2 CLIPPING{0.1F, 100.0F};

    /** What the sign over the well says. **/
    static constexpr const char* SIGN = "Village Well";

    /** The face the sign is set in, in the shared data directory. **/
    static constexpr const char* SIGN_FONT = "fonts/Inter-SemiBold.ttf";

    /** The sign's size, in its own pixels, and how long one of them is in the world. **/
    static constexpr float SIGN_SIZE = 64.0F;
    static constexpr float SIGN_SCALE = 1.0F / 80.0F;

    /** How far above the well's base the top of the sign stands, in world units. **/
    static constexpr float SIGN_HEIGHT = 3.2F;

    /** The sign's colour, in linear light. **/
    static constexpr glm::vec4 SIGN_COLOUR{1.0F, 0.95F, 0.8F, 1.0F};

    /**
     * Build the lit scene at a size, or replace its target with one of a new size, and give the
     * camera the window's aspect.
     **/
    void resize(uint32_t width, uint32_t height);

    /**
     * Stand the sign over the well in the lit pass, turned to face the camera.
     **/
    void drawSign(v3d::render::realtime::Pass* lit);

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
    // the sign over the well: its words laid out once, drawn from their curves, and the world
    // quads they become, built for the lit scene's target
    boost::shared_ptr<v3d::font::ShapingCache> shaping_;
    std::optional<v3d::font::Paragraph> sign_;
    boost::shared_ptr<v3d::ui::paint::CurveText> signText_;
    boost::shared_ptr<v3d::render::realtime::renderer::World> signs_;
};

};  // namespace diorama
