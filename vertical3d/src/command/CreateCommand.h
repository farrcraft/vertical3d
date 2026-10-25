/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Scene.h>

#include <string>

#include "Command.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 * An item of any kind put into the scene.
 *
 * The command holds the item rather than its id, so an item taken out of the scene by an
 * undo stays alive and comes back with the id it had. A transform command deeper in the
 * history names the same object, and would name nothing if the item had been rebuilt.
 **/
class CreateCommand final : public Command {
 public:
    /**
     * @param kind what the item is called, which is the name the command reports
     **/
    CreateCommand(const boost::shared_ptr<Scene>& scene,
        const boost::shared_ptr<Item>& item,
        const std::string& kind);

    void undo() override;
    void redo() override;
    std::string name() const override;

 private:
    boost::shared_ptr<Scene> scene_;
    boost::shared_ptr<Item> item_;
    std::string kind_;
};

};  // namespace v3d::editor
