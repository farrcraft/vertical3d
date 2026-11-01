/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Timeline.h>

#include <string>

#include "Command.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor::command {

/**
 * A change to the timeline's frame rate, range or auto key. The document saves them, so they
 * are undone like any other change to the document. No curve changes.
 *
 * The current frame is not part of the change. Undo and redo leave it where it is, held within
 * whichever range they put back.
 **/
class Timeline final : public Command {
 public:
    /**
     * @param wanted the rate, range and auto key once the change is done. Its frame is ignored
     * @param what what to call the change, such as "frame range"
     **/
    Timeline(const boost::shared_ptr<scene::Scene>& scene, const scene::Timeline& wanted, const std::string& what);

    void undo() override;
    void redo() override;
    std::string name() const override;

 private:
    /**
     * Give the scene one end's rate, range and auto key.
     **/
    void apply(const scene::Timeline& from);

    boost::shared_ptr<scene::Scene> scene_;
    scene::Timeline before_;
    scene::Timeline after_;
    std::string what_;
};

};  // namespace v3d::editor::command
