/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Keyed.h"

#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Properties.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Timeline.h>

#include <string>
#include <vector>

#include "Command.h"
#include "Key.h"

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor::command {

/**
 **/
boost::shared_ptr<Command> Keyed::record(const boost::shared_ptr<scene::Scene>& scene, const boost::shared_ptr<Command>& edit,
    const std::vector<Change>& changes) {
    if (!scene || !edit || !scene->timeline().autoKey) {
        return edit;
    }
    std::vector<Key::Target> targets;
    for (const Change& change : changes) {
        if (!change.item) {
            continue;
        }
        const scene::Item::Kind kind = change.item->kind();
        const Key::Target target{ .item = change.item,
            .paths = scene::changed(scene::values(kind, change.before, glm::vec3(0.0F)), scene::values(kind, change.after, glm::vec3(0.0F))) };
        if (!target.paths.empty()) {
            targets.push_back(target);
        }
    }
    const boost::shared_ptr<Key> key = boost::make_shared<Key>(scene, targets, static_cast<float>(scene->timeline().frame));
    if (key->empty()) {
        return edit;
    }
    key->redo();
    return boost::make_shared<Keyed>(edit, key);
}

/**
 **/
Keyed::Keyed(const boost::shared_ptr<Command>& edit, const boost::shared_ptr<Key>& key) :
    edit_(edit),
    key_(key) {
}

/**
 **/
void Keyed::undo() {
    key_->undo();
    edit_->undo();
}

/**
 **/
void Keyed::redo() {
    edit_->redo();
    key_->redo();
}

/**
 **/
std::string Keyed::name() const {
    return edit_->name();
}

};  // namespace v3d::editor::command
