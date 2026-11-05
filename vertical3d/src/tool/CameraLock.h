/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vertical3d/src/command/Edit.h>
#include <vertical3d/src/command/Stack.h>
#include <vertical3d/src/scene/item/Camera.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/view/ViewPort.h>

#include <optional>

#include <boost/shared_ptr.hpp>

namespace v3d::editor::tool {

/**
 * Moves the scene camera a view is locked to as the view is navigated, and records each drag as
 * one change.
 *
 * The camera goes where the view's eye is, facing the way it looks, beneath the parent it has.
 * The view's angle of view, or its orthographic extent, becomes the lens's, less the room the view
 * shows around the render's frame. While auto key is on, the drag keys what it changed.
 **/
class CameraLock final {
 public:
    /**
     * @param history where a drag is recorded when it ends
     **/
    explicit CameraLock(const boost::shared_ptr<command::Stack>& history);

    /**
     * @return a camera's values with its placement and lens where a view through it puts them
     **/
    static command::Edit::Values placed(const scene::Scene& scene, const scene::item::Camera& camera, const view::ViewPort& view);

    /**
     * Move the camera a view is locked to where the view now is. The first move of a drag
     * remembers the camera's values before it.
     **/
    void moved(const boost::shared_ptr<scene::Scene>& scene, const view::ViewPort& view);

    /**
     * Record the drag that has ended, if it moved anything.
     **/
    void ended();

 private:
    /**
     * A drag under way: the scene, the camera, and its values before the drag.
     **/
    struct Pending final {
        boost::shared_ptr<scene::Scene> scene;
        boost::shared_ptr<scene::item::Camera> camera;
        command::Edit::Values before;
    };

    boost::shared_ptr<command::Stack> history_;
    std::optional<Pending> pending_;
};

};  // namespace v3d::editor::tool
