/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <vector>

#include "manipulator/Manipulator.h"
#include "scene/Scene.h"
#include "view/ViewPort.h"

#include "../../api/asset/Manager.h"
#include "../../api/log/Logger.h"
#include "../../api/render/realtime/Canvas.h"
#include "../../api/render/realtime/Engine3D.h"
#include "../../api/render/realtime/LineCanvas.h"
#include "../../api/render/realtime/Window.h"
#include "../../api/ui/ComponentRenderer.h"
#include "../../api/ui/Engine.h"
#include "../../api/ui/TextRenderer.h"

#include <boost/shared_ptr.hpp>
#include <entt/entt.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

/**
 * The editor's frame: two passes per viewport, over one device.
 *
 * Four views of one scene is four pairs of passes with four cameras, per ADR-0003. Each
 * pass carries its viewport's region as its scissor and its camera at set 0, and the
 * scene pass clears its own region - so the split is a property of the frame rather than
 * of the window.
 *
 * A scene pass depth tests, because the wireframe a modeller draws has to be occluded by
 * what is in front of it; they all share one depth buffer, which each clears within its
 * own region. The handle pass that follows it does not, and keeps what the scene pass
 * left: the manipulator is an overlay, and an overlay is a pass without depth per
 * ADR-0011.
 *
 * One more pass follows all of them, over the whole window rather than a view, and holds
 * the ui - quads rather than lines, because a panel and a glyph are the same primitive
 * per ADR-0005.
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
     * The handles the selection carries, drawn by every view that shows them. Empty
     * when the transform tool is in the mode that draws none.
     **/
    void manipulator(const boost::shared_ptr<Manipulator>& manipulator);

    /**
     * The ui drawn over every view, or null to draw none.
     **/
    void ui(const boost::shared_ptr<v3d::ui::Engine>& ui);

    /**
     * How much of the window's edges the ui covers - the menu bar and the toolbars - which
     * is what the views are not given. Left in x and top in y.
     **/
    glm::vec2 insets() const;

    /**
     * Draw one frame - a pass per view.
     **/
    void draw();

    /**
     * Wait for everything in flight, before the window the device draws to goes away.
     **/
    void shutdown();

 private:
    /**
     * Fill the ui canvas and give the frame the one pass that draws it.
     **/
    void drawUi(const boost::shared_ptr<v3d::render::realtime::Frame>& frame);

    boost::shared_ptr<v3d::log::Logger> logger_;
    // kept for the images a ui theme names, which are resolved when the ui arrives
    boost::shared_ptr<v3d::asset::Manager> assetManager_;

    // first, so that everything holding a device handle below is destroyed before the
    // context that owns the device is
    v3d::render::realtime::Engine3D engine_;

    boost::shared_ptr<Scene> scene_;
    boost::shared_ptr<Manipulator> manipulator_;
    std::vector<boost::shared_ptr<ViewPort>> views_;
    // one canvas per view rather than one shared: a canvas becomes a single draw item,
    // and each is filled before any of them is submitted
    std::vector<v3d::render::realtime::LineCanvas> canvases_;
    // and one more per view for the handles, which are drawn in a pass of their own
    std::vector<v3d::render::realtime::LineCanvas> overlays_;

    boost::shared_ptr<v3d::ui::Engine> ui_;
    boost::shared_ptr<v3d::ui::ComponentRenderer> uiRenderer_;
    v3d::render::realtime::Canvas canvas_;
    boost::shared_ptr<v3d::ui::TextRenderer> text_;

    glm::vec4 background_;
};

};  // namespace v3d::editor
