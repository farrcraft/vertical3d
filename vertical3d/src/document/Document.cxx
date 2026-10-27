/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

#include "Document.h"

#include <api/log/Logger.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/command/Command.h>
#include <vertical3d/src/command/CommandStack.h>
#include <vertical3d/src/command/CreateCommand.h>
#include <vertical3d/src/command/GroupCommand.h>
#include <vertical3d/src/command/KeyCommand.h>
#include <vertical3d/src/command/SettingsCommand.h>
#include <vertical3d/src/command/TopologyCommand.h>
#include <vertical3d/src/render/Shot.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/CreateItem.h>
#include <vertical3d/src/scene/CreatePoly.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Light.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Modelling.h>
#include <vertical3d/src/scene/Project.h>
#include <vertical3d/src/scene/RIBExportVisitor.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Timeline.h>

#include <algorithm>
#include <fstream>
#include <string>

#include <boost/filesystem/path.hpp>
#include <boost/make_shared.hpp>

namespace v3d::editor {

namespace {

/**
 * What a modelling operation says when it changed the mesh, and when there was nothing to change.
 **/
struct Said final {
    std::string done;
    std::string refused;
};

/**
 * Record a modelling operation's change, or say why there was none. A modelling operation has
 * changed the mesh by the time it hands its record back.
 **/
Outcome record(CommandStack* history, const boost::shared_ptr<TopologyCommand>& command, const Said& said) {
    if (!command) {
        return { .done = false, .message = said.refused };
    }
    history->push(command);
    return { .done = true, .message = said.done };
}

};  // namespace

Document::Document(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::filesystem::path& project) :
    scene_(boost::make_shared<Scene>()),
    history_(boost::make_shared<CommandStack>()),
    project_(boost::make_shared<Project>(logger, project)),
    sinceAutosave_(0) {
}

const boost::shared_ptr<Scene>& Document::scene() const noexcept {
    return scene_;
}

const boost::shared_ptr<CommandStack>& Document::history() const noexcept {
    return history_;
}

const boost::shared_ptr<Project>& Document::project() const noexcept {
    return project_;
}

bool Document::dirty() const noexcept {
    return history_->dirty();
}

Outcome Document::create(const boost::shared_ptr<Item>& item, const std::string& name) {
    // the command does the creating, so making an item and redoing one run the same code
    const boost::shared_ptr<CreateCommand> command = boost::make_shared<CreateCommand>(scene_, item, name);
    command->redo();
    history_->push(command);
    return { .done = true, .message = "created a " + name + " - " + std::to_string(scene_->count()) + " items" };
}

Outcome Document::createPrimitive(Primitive primitive) {
    switch (primitive) {
        case Primitive::Cube:
            return create(boost::make_shared<Mesh>(create_poly_cube()), "cube");
        case Primitive::Plane:
            return create(boost::make_shared<Mesh>(create_poly_plane()), "plane");
        case Primitive::Cylinder:
            return create(boost::make_shared<Mesh>(create_poly_cylinder()), "cylinder");
        case Primitive::Cone:
            return create(boost::make_shared<Mesh>(create_poly_cone()), "cone");
    }
    return {};
}

Outcome Document::createCamera(const v3d::type::camera::Profile& at, Camera::Projection projection) {
    const char* name = projection == Camera::Projection::Orthographic ? "orthographic camera" : "perspective camera";
    return create(create_camera(at, projection), name);
}

Outcome Document::createLight(const v3d::type::camera::Profile& at, Light::Type type) {
    const char* name = "point light";
    if (type == Light::Type::Distant) {
        name = "distant light";
    } else if (type == Light::Type::Spot) {
        name = "spot light";
    }
    return create(create_light(at, type), name);
}

Outcome Document::group(const Items& items) {
    const boost::shared_ptr<GroupCommand> command = boost::make_shared<GroupCommand>(scene_, items);
    if (command->empty()) {
        return { .done = false, .message = "a group gathers items, and none was given" };
    }
    command->redo();
    history_->push(command);
    return { .done = true, .message = "grouped " + std::to_string(items.size()) + " items - " + std::to_string(scene_->count()) + " items" };
}

Outcome Document::key(const Items& items, int frame) {
    const boost::shared_ptr<KeyCommand> command = boost::make_shared<KeyCommand>(scene_, items, static_cast<float>(frame));
    if (command->empty()) {
        return { .done = false, .message = "a key is set on items, and none was given" };
    }
    command->redo();
    history_->push(command);
    return { .done = true, .message = "set a key at frame " + std::to_string(frame) };
}

Outcome Document::settings(const RenderSettings& settings) {
    if (settings == scene_->renderSettings()) {
        return { .done = false, .message = "the render settings are unchanged" };
    }
    const boost::shared_ptr<SettingsCommand> command = boost::make_shared<SettingsCommand>(scene_, settings);
    command->redo();
    history_->push(command);
    return { .done = true, .message = "changed the render settings" };
}

Outcome Document::splitEdges() {
    return record(history_.get(), editor::splitEdges(*scene_),
        {.done = "split the selected edges", .refused = "split edge: no edge is selected"});
}

Outcome Document::splitFace() {
    std::string problem;
    const boost::shared_ptr<TopologyCommand> command = editor::splitFace(*scene_, &problem);
    return record(history_.get(), command, {.done = "split a face between the selected vertices", .refused = "split face: " + problem});
}

Outcome Document::mergeVertices() {
    std::string problem;
    const boost::shared_ptr<TopologyCommand> command = editor::mergeVertices(*scene_, &problem);
    return record(history_.get(), command, {.done = "merged the selected vertices", .refused = "merge vertices: " + problem});
}

Outcome Document::extrudeFaces() {
    return record(history_.get(), editor::extrudeFaces(*scene_),
        {.done = "extruded the selected faces", .refused = "extrude face: no face is selected"});
}

Outcome Document::insetFaces() {
    return record(history_.get(), editor::insetFaces(*scene_),
        {.done = "inset the selected faces", .refused = "inset face: no face is selected"});
}

Outcome Document::undo() {
    const boost::shared_ptr<Command> command = history_->undo();
    if (!command) {
        return { .done = false, .message = "nothing to undo" };
    }
    return { .done = true, .message = "undo " + command->name() };
}

Outcome Document::redo() {
    const boost::shared_ptr<Command> command = history_->redo();
    if (!command) {
        return { .done = false, .message = "nothing to redo" };
    }
    return { .done = true, .message = "redo " + command->name() };
}

void Document::goTo(int frame) {
    const Timeline& timeline = scene_->timeline();
    scene_->frame(std::clamp(frame, timeline.start, timeline.end));
}

bool Document::open(const boost::filesystem::path& manifest) {
    if (!project_->open(manifest, scene_)) {
        return false;
    }
    sinceAutosave_ = 0;
    // the history describes the scene that was replaced, so none of it can be undone against
    // the new one. Clearing it also marks the new scene as saved
    history_->clear();
    return true;
}

bool Document::save() {
    if (!project_->save(scene_)) {
        return false;
    }
    history_->markSaved();
    return true;
}

bool Document::saveAs(const boost::filesystem::path& directory) {
    if (!project_->saveAs(directory, scene_)) {
        return false;
    }
    history_->markSaved();
    return true;
}

void Document::tick(unsigned int delta) {
    sinceAutosave_ += delta;
    if (sinceAutosave_ < autosaveInterval) {
        return;
    }
    sinceAutosave_ = 0;
    if (history_->dirty()) {
        project_->autosave(scene_);
    }
}

bool Document::recoverable() const {
    return project_->recoverable();
}

bool Document::recover() {
    if (!project_->recover(scene_)) {
        return false;
    }
    // what was recovered is not what the project holds
    history_->clear();
    history_->markUnsaved();
    return true;
}

void Document::discardRecovery() {
    project_->discardRecovery();
}

void Document::close() {
    if (!history_->dirty()) {
        project_->discardRecovery();
    }
}

Outcome Document::exportRib(const boost::filesystem::path& file, const v3d::type::camera::Profile& from,
    const std::string& picture) {
    std::ofstream stream(file.string().c_str());
    if (!stream.is_open()) {
        return { .done = false, .message = "could not write " + file.string() };
    }
    writeShot(&stream, *scene_, scene_->evaluate(), from, RIBExportVisitor::Output{ .settings = scene_->renderSettings(), .file = picture });
    return { .done = true, .message = "exported " + std::to_string(scene_->count()) + " item(s) to " + file.string() };
}

};  // namespace v3d::editor
