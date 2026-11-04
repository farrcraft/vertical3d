/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/canvas/Line.h>
#include <api/type/camera/Camera.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/selection/SelectMask.h>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "ConstructionPlane.h"
#include "Shading.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor::view {


/**
 * One view of the scene: a camera, the region of the window it draws into, and what it
 * decorates the scene with.
 *
 * A viewport is not a window and owns no device state. The renderer has it fill a
 * canvas::Line each frame, and turns that into a pass with this viewport's camera at set 0
 * and this viewport's region. Every view of the scene therefore draws through one device.
 **/
class ViewPort final {
 public:
    /**
     * Bit flags, ORed together into showFlags_. Each value owns a bit of its own, or
     * toggling one decoration would hide another.
     **/
    typedef enum VisibleFilter : std::uint8_t {
        SHOW_GRID = (1 << 0),
        SHOW_CAMERA = (1 << 1),
        SHOW_HANDLE = (1 << 2),
        SHOW_LIGHT = (1 << 3),
        SHOW_MESH = (1 << 4)
    } VisibleFilter;

    /**
     * Which side of a mesh's triangles the shaded pass leaves out. Front is the side a face's
     * outward normal points to.
     **/
    enum class Cull : std::uint8_t { None, Front, Back, Both };

    /**
     * How the shaded pass draws one side of the triangles: as points at their corners, as their
     * edges, or filled.
     **/
    enum class Fill : std::uint8_t { Points, Lines, Fill };

    /**
     * How the view shades the scene's meshes, under its wireframe.
     *
     * The front of each triangle is drawn in the front fill and the back in the back fill,
     * unless the cull leaves that side out. The default draws the front filled and leaves the
     * back out, because the back of a closed surface is never seen.
     **/
    struct Display {
        Shading shading = Shading::Flat;
        Cull cull = Cull::Back;
        Fill front = Fill::Fill;
        Fill back = Fill::Fill;
    };

    /**
     * The fixed view a view through a scene camera starts from, and is named for once it is
     * freed.
     **/
    static constexpr std::string_view FREE_VIEW = "Perspective";

    /**
     * What the view looks through. A free view has a camera of its own, which navigation moves.
     * Otherwise it looks through a scene camera: the one it names, or the scene's camera, which
     * is the first selected camera, else the first in the scene.
     **/
    struct Through final {
        std::int64_t camera = 0;  /**< the scene camera's document id, or 0 for none **/
        bool scene = false;       /**< whether it follows the scene's camera rather than one it names **/
        bool locked = false;      /**< whether navigation moves the scene camera, rather than freeing the view **/
    };

    /**
     * @param name what the view is called, which is also its pass name
     * @param profile the camera profile the view starts from
     **/
    ViewPort(const std::string& name, const v3d::type::camera::Profile& profile);

    /**
     * @return the camera the view draws through, which the camera tools drive
     **/
    boost::shared_ptr<v3d::type::camera::Camera> camera() const;

    /**
     * @return the view's name, which is also the name of the pass it draws into
     **/
    const std::string& name() const noexcept;

    /**
     * Give the view the region of the window it covers, as x, y, width, height.
     *
     * The camera's profile gets the same size and its pixel aspect ratio. The ortho factors
     * divide by the viewport size, and the projection is built around the pixel aspect
     * ratio, so a camera that has not been given a region drives at zero units per pixel.
     **/
    void resize(const glm::vec4& region);

    /**
     * Show the scene through another camera profile, such as one of the fixed views.
     *
     * The camera stays the same object, so a tool that drives it keeps driving it. The view
     * keeps its region, so the profile is given the view's size and pixel aspect ratio.
     **/
    void look(const v3d::type::camera::Profile& profile);

    /**
     * @return the region of the window the view covers
     **/
    const glm::vec4& region() const noexcept;

    /**
     * What the view looks through. Setting it changes nothing shown until the view is next
     * followed, which sets its camera from the scene's, except that a free view drops its frame.
     **/
    const Through& through() const noexcept;
    void through(const Through& through) noexcept;

    /**
     * @return whether the view is free: it looks through no scene camera
     **/
    bool free() const noexcept;

    /**
     * The document id of the scene camera the view showed when it was last followed, or 0 for
     * none. A view that follows the scene's camera shows a different one as the selection
     * changes.
     **/
    std::int64_t looking() const noexcept;
    void looking(std::int64_t camera) noexcept;

    /**
     * Stop looking through a scene camera, and show what it showed as a free view named
     * Perspective. Navigation then moves the view's own camera.
     **/
    void letGo();

    /**
     * The part of the view the render's picture fills, as x, y, width and height from the view's
     * top left corner, while the view looks through a scene camera. The view dims what is
     * outside it. Nothing for a free view.
     **/
    const std::optional<glm::vec4>& frame() const noexcept;
    void frame(const std::optional<glm::vec4>& frame) noexcept;

    /**
     * Which decorations the view draws.
     **/
    void show(unsigned int flags) noexcept;

    /**
     * @return which decorations the view draws
     **/
    unsigned int show() const noexcept;

    /**
     * Turn one decoration on or off, leaving the rest alone.
     **/
    void show(VisibleFilter filter, bool visible) noexcept;

    /**
     * @return whether one decoration is drawn
     **/
    bool shows(VisibleFilter filter) const noexcept;

    /**
     * How the view shades the meshes it shows. Each view has its own.
     **/
    const Display& display() const noexcept;
    Display& display() noexcept;

    /**
     * @return the grid this view measures against
     **/
    ConstructionPlane& grid() noexcept;

    /**
     * Rebuild the view's geometry for this frame, into the canvas the renderer will
     * hand to the line primitive.
     *
     * The canvas is cleared first. It is rebuilt every frame, because the grid follows the
     * camera and the camera moves.
     *
     * Every view that shows meshes draws the scene into its own canvas, through its own
     * camera. The camera's matrices are rebuilt first, so whatever else draws through the
     * camera this frame, such as the handles, sees the same view. An item the views do not show
     * is not drawn.
     *
     * @param scene what to draw, which the controller owns
     * @param canvas where the view's geometry goes
     * @param mask the select mask in force. Under Vertex and Handle a selected curve shows its
     *        control points, and its Bezier handles while the view shows handles
     **/
    void draw(const scene::Scene& scene, v3d::render::realtime::canvas::Line* canvas, selection::SelectMask mask = selection::SelectMask::Object);

 private:
    std::string name_;
    boost::shared_ptr<v3d::type::camera::Camera> camera_;
    ConstructionPlane grid_;
    glm::vec4 region_;
    unsigned int showFlags_;
    Display display_;
    Through through_;
    std::int64_t looking_ = 0;
    std::optional<glm::vec4> frame_;
};

};  // namespace v3d::editor::view
