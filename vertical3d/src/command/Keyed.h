/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Properties.h>
#include <vertical3d/src/scene/Scene.h>

#include <string>
#include <vector>

#include "Command.h"
#include "Key.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor::command {

/**
 * An edit of items' values and the key auto key set with it, which undo takes back together.
 **/
class Keyed final : public Command {
 public:
    /**
     * One item an edit changed, and its pose at both ends of the edit.
     **/
    struct Change {
        boost::shared_ptr<scene::Item> item;
        scene::Pose before;
        scene::Pose after;
    };

    /**
     * Record an edit, keyed when auto key is on. The edit is already made. The key is made here,
     * at the current frame, on each property the edit changed.
     *
     * @return the edit alone when auto key is off or the edit changed nothing a curve can drive,
     *         and otherwise the edit with its key
     **/
    static boost::shared_ptr<Command> record(const boost::shared_ptr<scene::Scene>& scene, const boost::shared_ptr<Command>& edit,
        const std::vector<Change>& changes);

    /**
     * @param edit a change already made
     * @param key the key set with it, already set
     **/
    Keyed(const boost::shared_ptr<Command>& edit, const boost::shared_ptr<Key>& key);

    void undo() override;
    void redo() override;

    /**
     * @return the edit's name, which the key does not change
     **/
    std::string name() const override;

 private:
    boost::shared_ptr<Command> edit_;
    boost::shared_ptr<Key> key_;
};

};  // namespace v3d::editor::command
