/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/LineCanvas.h>
#include <api/type/camera/Camera.h>
#include <vertical3d/src/scene/Scene.h>

#include <string>

#include "ConstructionPlane.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

class Manipulator;

/**
 * One view of the scene: a camera, the region of the window it draws into, and what it
 * decorates the scene with.
 *
 * A viewport is not a window and owns no device state. It fills a LineCanvas during a
 * tick and the renderer turns that into a pass with this viewport's camera at set 0 and
 * this viewport's region as its scissor - which is what makes four views of one scene
 * four passes against one device, per ADR-0003.
 **/
class ViewPort final {
 public:
    /**
     * Bit flags, ORed together into showFlags_ - each value has to own a bit of
     * its own or toggling one decoration hides another.
     **/
    typedef enum VisibleFilter {
        SHOW_GRID = (1 << 0),
        SHOW_CAMERA = (1 << 1),
        SHOW_HANDLE = (1 << 2),
        SHOW_LIGHT = (1 << 3),
        SHOW_MESH = (1 << 4)
    } VisibleFilter;

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
     * The camera is told the same thing: the ortho factors divide by the viewport size
     * and the projection is built around the pixel aspect ratio, so a camera that has
     * not been given a region drives at zero units per pixel.
     **/
    void resize(const glm::vec4& region);

    /**
     * @return the region of the window the view covers
     **/
    const glm::vec4& region() const noexcept;

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
     * @return the grid this view measures against
     **/
    ConstructionPlane& grid() noexcept;

    /**
     * Rebuild the view's geometry for this frame, into the canvas the renderer will
     * hand to the line primitive.
     *
     * The canvas is cleared first: it is rebuilt every frame rather than kept, because
     * the grid follows the camera and the camera moves.
     *
     * The scene is drawn by every view that shows meshes, so one scene becomes four
     * canvases - each view draws it through its own camera.
     *
     * The handles go to a canvas of their own because they are an overlay: they are
     * drawn in a second pass that does not depth test, per ADR-0011. Sharing the scene's
     * canvas would put a handle in the depth tested pass, where the grid's own axis
     * lines lie in the same plane and win.
     *
     * @param scene what to draw, which the controller owns
     * @param manipulator the handles the selection carries, which every view that shows
     *        handles draws through its own camera - may be null
     * @param canvas where the view's geometry goes
     * @param handles where the manipulator's geometry goes - may be null
     **/
    void draw(const Scene& scene, const Manipulator* manipulator, v3d::render::realtime::LineCanvas* canvas,
        v3d::render::realtime::LineCanvas* handles);

 private:
    std::string name_;
    boost::shared_ptr<v3d::type::camera::Camera> camera_;
    ConstructionPlane grid_;
    glm::vec4 region_;
    unsigned int showFlags_;
};

};  // namespace v3d::editor
