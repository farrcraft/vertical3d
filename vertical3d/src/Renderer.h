/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Manager.h>
#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/render/realtime/Engine3D.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/render/realtime/canvas/Line.h>
#include <api/shell/Shell.h>
#include <api/ui/Engine.h>
#include <api/ui/paint/TextRenderer.h>
#include <api/ui/shell/Screen.h>
#include <api/window/Window.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/tool/SelectTool.h>
#include <vertical3d/src/tool/TransformTool.h>
#include <vertical3d/src/view/ShadedPass.h>
#include <vertical3d/src/view/ViewPort.h>

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <entt/entt.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

/**
 * The editor's frame: two passes per viewport, over one device.
 *
 * Each view of the scene is a pair of passes with the view's own camera. Each pass carries
 * its viewport's region and its camera at set 0, and the scene pass clears only its own
 * region. The split into views is therefore a property of the frame, not of the window.
 *
 * A scene pass depth tests, so a wireframe is occluded by what is in front of it. All the
 * scene passes share one depth buffer, and each clears it within its own region. A view that
 * shows meshes draws their shaded surfaces in its scene pass first, and its wireframe over
 * them. The handle pass that follows does not depth test and keeps what the scene pass left,
 * because the manipulator is an overlay.
 *
 * One more pass follows all of them. It covers the whole window, not one view, and draws
 * the ui. It draws quads, not lines, because panels and glyphs both go through the batched
 * quad pipeline.
 */
class Renderer final {
 public:
    /**
     * @param shell the editor's shell, which builds its screen over this renderer's engine.
     *        It outlives the renderer
     * @throw std::runtime_error if the device or the line pipelines cannot be built
     **/
    Renderer(const boost::shared_ptr<v3d::window::Window>& window,
        const boost::shared_ptr<v3d::log::Logger>& logger,
        const boost::shared_ptr<v3d::asset::Manager>& assetManager, v3d::shell::Shell* shell);

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
     * The one scene every view draws.
     **/
    void scene(const boost::shared_ptr<Scene>& scene);

    /**
     * The transform tool, whose manipulator every view that shows handles draws about what
     * the tool would move. It draws none in the mode that has no manipulator.
     **/
    void transformTool(const boost::shared_ptr<TransformTool>& transform);

    /**
     * The select tool, whose rubber band is drawn over the views while one is dragged.
     **/
    void selectTool(const boost::shared_ptr<SelectTool>& select);

    /**
     * The ui drawn over every view, or null to draw none.
     **/
    void ui(const boost::shared_ptr<v3d::ui::Engine>& ui);

    /**
     * How wide a run of text is at the size the ui is drawn in.
     *
     * The cursor routing a press must measure text with the same function this draws
     * with, or a click inside a text box would place the caret somewhere other than under
     * the pointer. Taking it from the renderer guarantees the two are the same function.
     **/
    v3d::ui::paint::Measure measure() const;

    /**
     * How much of the window's edges the menu bar and the toolbars cover. The views are
     * laid out in the rest. Left in x and top in y.
     **/
    glm::vec2 insets() const;

    /**
     * Draw one frame: two passes per view, then the ui pass.
     **/
    void draw();

    /**
     * Upload a rendered picture as a texture the ui can draw, in place of the last one.
     *
     * @return the texture, or an unset handle for no picture
     **/
    v3d::render::realtime::TextureHandle picture(const boost::shared_ptr<v3d::image::Image>& image);

    /**
     * Wait for everything in flight, before the window the device draws to goes away.
     **/
    void shutdown();

 private:
    /**
     * Fill the ui canvas and add the pass that draws it to the frame.
     **/
    void drawUi(const boost::shared_ptr<v3d::render::realtime::Frame>& frame);

    boost::shared_ptr<v3d::log::Logger> logger_;
    // kept for the images a ui theme names, which are resolved when the ui arrives
    boost::shared_ptr<v3d::asset::Manager> assetManager_;

    // first, so that everything holding a device handle below is destroyed before the
    // context that owns the device is
    v3d::render::realtime::Engine3D engine_;
    // the meshes' surfaces, built once the engine has a device
    boost::shared_ptr<ShadedPass> shaded_;

    boost::shared_ptr<Scene> scene_;
    boost::shared_ptr<TransformTool> transform_;
    boost::shared_ptr<SelectTool> select_;
    std::vector<boost::shared_ptr<ViewPort>> views_;
    // one canvas per view. Each view fills its own and submits it before the next view
    // draws, and the submit copies it
    std::vector<v3d::render::realtime::canvas::Line> canvases_;
    // one more canvas per view for the handles, which are drawn in a pass of their own
    std::vector<v3d::render::realtime::canvas::Line> overlays_;

    boost::shared_ptr<v3d::ui::Engine> ui_;
    v3d::shell::Shell* shell_;
    // the shell's screen, built over engine_
    v3d::ui::shell::Screen* screen_ = nullptr;

    glm::vec4 background_;
    // the last rendered picture's texture, released when the next one arrives
    v3d::render::realtime::TextureHandle picture_;
};

};  // namespace v3d::editor
