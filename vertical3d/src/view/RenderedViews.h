/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/image/Image.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/render/realtime/resource/Handle.h>
#include <vertical3d/src/render/Delegate.h>
#include <vertical3d/src/scene/Scene.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "ViewPort.h"

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::editor::view {

/**
 * The render sessions of the views in rendered shading: one for each view, rendering the scene
 * from the view's camera at the view's size divided by its pixel size.
 *
 * **A shown view's session follows the view.** Each tick hands it the scene and the view's
 * camera, so an orbit, a pan or a zoom is an edit of that session alone. Each new picture is
 * handed on under the view's name.
 *
 * **A view that is not shown pauses its session**, which keeps its picture and samples on once
 * the view is shown again. A view that leaves rendered shading, or is gone, ends its session, and
 * its picture is handed on as empty.
 **/
class RenderedViews final {
 public:
    /**
     * Make the renderer of one view's session.
     **/
    typedef std::function<boost::shared_ptr<render::Delegate>()> Make;

    /**
     * Take a view's latest picture, or an empty pointer once the view has none.
     **/
    typedef std::function<void(const std::string& view, const boost::shared_ptr<v3d::image::Image>& picture)> Show;

    RenderedViews(const Make& make, const Show& show);
    ~RenderedViews() = default;
    RenderedViews(const RenderedViews&) = delete;
    RenderedViews& operator=(const RenderedViews&) = delete;

    /**
     * Bring each rendered view's session up to date, and hand on each new picture.
     *
     * @param shown every view a window shows. A view that is in none of them is not shown.
     **/
    void tick(const scene::Scene& scene, const std::vector<boost::shared_ptr<ViewPort>>& shown);

    /**
     * End every session and wait for its thread, before what the pictures go to is destroyed.
     **/
    void release();

    /**
     * @return how many views have a session, paused or not
     **/
    std::size_t sessions() const noexcept;

    /**
     * @return the renderer of a view's session, or an empty pointer for a view with none
     **/
    boost::shared_ptr<render::Delegate> session(const std::string& view) const;

    /**
     * The size of a view's picture: the part of the view the render fills, divided by the view's
     * pixel size and rounded up. That part is the whole of a free view, and the camera's frame
     * of a view through a scene camera. Each side is at least one pixel and at most
     * scene::RenderSettings::LARGEST_SIDE.
     **/
    static glm::uvec2 resolution(const ViewPort& view);

    /**
     * Fill a canvas the size of a view with its picture, over the part of the view the render
     * fills. Drawn in the view's pass after its surfaces and before its lines, the picture
     * replaces the surfaces' colour and leaves their depth, which the lines are tested against.
     **/
    static void cover(const ViewPort& view, const v3d::render::realtime::resource::TextureHandle& picture, v3d::render::realtime::canvas::Canvas* canvas);

 private:
    /**
     * One view's session.
     **/
    struct Held {
        boost::weak_ptr<ViewPort> view;
        boost::shared_ptr<render::Delegate> delegate;
        // the count of the delegate's last picture handed on, and whether the view's absence
        // stopped the render
        std::uint64_t taken = 0;
        bool paused = false;
    };

    /**
     * End a view's session, and hand on that it has no picture.
     *
     * @return the session after it
     **/
    std::map<std::string, Held>::iterator end(const std::map<std::string, Held>::iterator& held);

    Make make_;
    Show show_;
    std::map<std::string, Held> held_;
};

};  // namespace v3d::editor::view
