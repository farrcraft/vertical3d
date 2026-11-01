/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Manager.h>
#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/render/realtime/WindowRenderer.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/render/realtime/canvas/Line.h>
#include <api/shell/Shell.h>
#include <api/ui/Image.h>
#include <api/ui/Immediate.h>
#include <api/ui/Interface.h>
#include <api/ui/paint/TextRenderer.h>
#include <api/ui/shell/Screen.h>
#include <api/window/Window.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/control/IconSheet.h>
#include <vertical3d/src/control/Notices.h>
#include <vertical3d/src/control/Pointer.h>
#include <vertical3d/src/control/StatusBar.h>
#include <vertical3d/src/tool/Select.h>
#include <vertical3d/src/tool/Transform.h>
#include <vertical3d/src/view/AreaViews.h>
#include <vertical3d/src/view/ConstructionPlane.h>
#include <vertical3d/src/view/ShadedPass.h>
#include <vertical3d/src/view/ViewOverlay.h>
#include <vertical3d/src/view/ViewPort.h>

#include <functional>
#include <map>
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
 * each view's axis marker and gizmo, the frame around the areas, a notice, the status bar and
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
     * The views to draw. Each frame draws the ones shown, in the order their areas are.
     **/
    void views(const boost::shared_ptr<view::AreaViews>& views);

    /**
     * What decides the frame drawn around the areas: the active area and the lit divider. The
     * frame is drawn under the ui and over the views.
     **/
    void pointer(const boost::shared_ptr<control::Pointer>& pointer);

    /**
     * What the status bar along the foot of the window says this frame.
     **/
    void status(const control::Status& status);

    /**
     * The notices drawn at the foot of the active area, or null for none.
     **/
    void notices(const boost::shared_ptr<control::Notices>& notices);

    /**
     * The one scene every view draws.
     **/
    void scene(const boost::shared_ptr<scene::Scene>& scene);

    /**
     * The transform tool, whose manipulator every view that shows handles draws about what
     * the tool would move. It draws none in the mode that has no manipulator.
     **/
    void transformTool(const boost::shared_ptr<tool::Transform>& transform);

    /**
     * The select tool, whose rubber band is drawn over the views while one is dragged.
     **/
    void selectTool(const boost::shared_ptr<tool::Select>& select);

    /**
     * The sheet an image source naming an icon is resolved from. Set it before the ui, whose
     * images are resolved when it arrives.
     **/
    void icons(const control::IconSheet& sheet);

    /**
     * The ui drawn over every view, or null to draw none.
     **/
    void ui(const boost::shared_ptr<v3d::ui::Interface>& ui);

    /**
     * Draws the panels of the areas that show no view into the ui canvas, through the
     * immediate layer, under the frame around the areas and the ui.
     **/
    typedef std::function<void(v3d::ui::Immediate* layer, v3d::render::realtime::canvas::Canvas* canvas)> Panels;

    /**
     * What draws the panels each frame, or empty for none.
     **/
    void panels(const Panels& draw);

    /**
     * @return the immediate layer the panels draw through, which the keyboard offers keys to
     **/
    v3d::ui::Immediate* immediate() const noexcept;

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
     * @return how much larger than its logical size the ui is drawn
     **/
    float scale() const noexcept;

    /**
     * @return how tall a menu bar, a toolbar, a tab or the status bar is, at the scale the ui
     *         is drawn
     **/
    float barHeight() const;

    /**
     * @return where each view's axis marker and gizmo sit, at the scale the ui is drawn and
     *         under a header
     **/
    view::OverlayMetrics overlay() const;

    /**
     * Resolve the images of a component added to the ui after it was handed over. Each image
     * is uploaded once, however often it is asked for.
     **/
    void resolve(const boost::shared_ptr<v3d::ui::Component>& component);

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
     * What every viewport clears to.
     **/
    static constexpr glm::vec4 background = glm::vec4(0.16F, 0.17F, 0.19F, 1.0F);

    /**
     * The outline of a rubber band.
     **/
    static constexpr glm::vec4 bandColour = glm::vec4(0.9F, 0.9F, 0.9F, 1.0F);

    /**
     * How opaque the accent laid over where a dragged tab would land is.
     **/
    static constexpr float landingAlpha = 0.25F;

    /**
     * Where a line of text's baseline sits below the middle of the band it is centred in, as a
     * share of the text's size.
     **/
    static constexpr float BASELINE = 0.35F;

    /**
     * How far a notice sits above the foot of its area, at a ui scale of 1.
     **/
    static constexpr float NOTICE_MARGIN = 16.0F;

    /**
     * How much stronger an emphasised grid line is than an ordinary one, and how strongly the
     * lines through the origin show their axis colours.
     **/
    static constexpr float GRID_MAJOR = 2.0F;
    static constexpr float GRID_AXIS = 0.75F;

    /**
     * What separates the parts of the right side of the status bar.
     **/
    static constexpr const char* STATUS_GAP = "     ";

    /**
     * What a view's handle pass is called, on the end of the view's own name.
     **/
    static constexpr const char* handleSuffix = " handles";

    /**
     * The pass that belongs to no view, drawn over all of them.
     **/
    static constexpr const char* uiPass = "ui";

    /**
     * How large the whole ui is drawn, as a multiple of the size it was laid out at. The screen
     * multiplies it by the monitor's scale.
     *
     * Every metric below is a multiple of the text size, and the text size is a multiple of
     * this, so the editor scales by one number. Glyphs are distance fields, so a new scale
     * does not need a rebuilt atlas.
     **/
    static constexpr float uiScale = 1.0F;

    /**
     * The size the ui text is drawn at, and what the bar and its rows are sized from.
     **/
    static constexpr float fontSize = 12.0F * uiScale;

    /**
     * How large a toolbar's icon is, as a multiple of the text size: 18 pixels beside 12 pixel
     * text.
     **/
    static constexpr float iconScale = 1.5F;

    /**
     * The ui's spacing, each a multiple of the text size. At 12 pixel text a row is 24 pixels,
     * a header or bar 28, the gap around text 12, and the gap above and below a dropped panel's
     * items 4. An icon button is as wide as its icon and the gap, 30 pixels.
     **/
    static constexpr float lineHeightScale = 2.0F;
    static constexpr float paddingScale = 1.0F;
    static constexpr float barHeightScale = 28.0F / 12.0F;
    static constexpr float panelPaddingScale = 1.0F / 3.0F;

    /**
     * @return what a ui image source resolves to. An icon's name is a cell of the icon sheet.
     *         Any other source is the whole of the image it names, or nothing when the image
     *         cannot be loaded
     **/
    v3d::ui::Image image(const std::string& source);

    /**
     * @return the texture an image becomes, uploading it the first time it is asked for, or an
     *         unset handle when the image cannot be loaded
     **/
    v3d::render::realtime::TextureHandle texture(const std::string& source);

    /**
     * Fill the ui canvas and add the pass that draws it to the frame.
     **/
    void drawUi(const boost::shared_ptr<v3d::render::realtime::Frame>& frame);

    /**
     * @return the colours of a view's grid: the theme's hairline over the view's background,
     *         twice as strong on an emphasised line, and the axis colours on the lines through
     *         the origin
     **/
    view::ConstructionPlane::Colours gridColours() const;

    /**
     * Keep the frame's passes to the views shown. A pass stays in the frame and clears its
     * region every frame until it is removed, so the passes of a view no longer shown are
     * removed. A new view's pass is added after the ui pass, so the ui pass is removed too, and
     * is made again after every view's.
     **/
    void retire(v3d::render::realtime::Frame* frame, const std::vector<boost::shared_ptr<view::ViewPort>>& views);

    /**
     * Draw the status bar across the foot of the canvas, beside any toolbar down the left: what
     * each button does on the left, and the selection, the frame and the frame rate on the right.
     **/
    void drawStatus(v3d::render::realtime::canvas::Canvas* canvas) const;

    /**
     * Draw the notice showing, if there is one, on a plate centred at the foot of the active
     * area and kept inside the areas, faded as far as it has faded.
     **/
    void drawNotice(v3d::render::realtime::canvas::Canvas* canvas);

    boost::shared_ptr<v3d::log::Logger> logger_;
    // kept for the images a ui theme names, which are resolved when the ui arrives
    boost::shared_ptr<v3d::asset::Manager> assetManager_;

    // first, so that everything holding a device handle below is destroyed before the
    // context that owns the device is
    v3d::render::realtime::WindowRenderer engine_;
    // the meshes' surfaces, built once the engine has a device
    boost::shared_ptr<view::ShadedPass> shaded_;

    boost::shared_ptr<scene::Scene> scene_;
    boost::shared_ptr<tool::Transform> transform_;
    boost::shared_ptr<tool::Select> select_;
    boost::shared_ptr<view::AreaViews> views_;
    boost::shared_ptr<control::Pointer> pointer_;
    control::Status status_;
    boost::shared_ptr<control::Notices> notices_;
    // one canvas per view shown. Each view fills its own and submits it before the next view
    // draws, and the submit copies it
    std::vector<v3d::render::realtime::canvas::Line> canvases_;
    // one more canvas per view for the handles, which are drawn in a pass of their own
    std::vector<v3d::render::realtime::canvas::Line> overlays_;
    // the names of the views the frame holds passes for
    std::vector<std::string> viewPasses_;

    boost::shared_ptr<v3d::ui::Interface> ui_;
    Panels panels_;
    control::IconSheet icons_;
    // the textures the ui's images became, by source, so a component built again reuses them
    std::map<std::string, v3d::render::realtime::TextureHandle> images_;
    v3d::shell::Shell* shell_;
    // the shell's screen, built over engine_
    v3d::ui::shell::Screen* screen_ = nullptr;

    glm::vec4 background_;
    // the last rendered picture's texture, released when the next one arrives
    v3d::render::realtime::TextureHandle picture_;
};

};  // namespace v3d::editor
