/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vertical3d/src/scene/Scene.h>

#include <string>

#include "Command.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor::command {

/**
 * Turning auto key on or off. The document saves it, so it is undone like any other change to
 * the document. No curve changes.
 **/
class AutoKey final : public Command {
 public:
    /**
     * @param on whether an edit sets a key once the change is done
     **/
    AutoKey(const boost::shared_ptr<scene::Scene>& scene, bool on);

    void undo() override;
    void redo() override;
    std::string name() const override;

 private:
    /**
     * Set auto key, and leave the rest of the timeline as it is.
     **/
    void apply(bool on);

    boost::shared_ptr<scene::Scene> scene_;
    bool on_;
};

};  // namespace v3d::editor::command
