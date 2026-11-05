/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

#include "Document.h"

#include <api/animation/Curve.h>
#include <api/brep/Primitives.h>
#include <api/interchange/Gltf.h>
#include <api/interchange/Scene.h>
#include <api/interchange/usd/Usd.h>
#include <api/log/Logger.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/command/Command.h>
#include <vertical3d/src/command/Create.h>
#include <vertical3d/src/command/Group.h>
#include <vertical3d/src/command/Import.h>
#include <vertical3d/src/command/Key.h>
#include <vertical3d/src/command/Keys.h>
#include <vertical3d/src/command/Modelling.h>
#include <vertical3d/src/command/Settings.h>
#include <vertical3d/src/command/Show.h>
#include <vertical3d/src/command/Stack.h>
#include <vertical3d/src/command/Timeline.h>
#include <vertical3d/src/command/Topology.h>
#include <vertical3d/src/render/RIBExportVisitor.h>
#include <vertical3d/src/render/Shot.h>
#include <vertical3d/src/scene/item/Camera.h>
#include <vertical3d/src/scene/item/CreateItem.h>
#include <vertical3d/src/scene/Import.h>
#include <vertical3d/src/scene/item/Item.h>
#include <vertical3d/src/scene/item/Light.h>
#include <vertical3d/src/scene/item/Mesh.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Timeline.h>
#include <vertical3d/src/scene/Units.h>
#include <vertical3d/src/store/Project.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <fstream>
#include <set>
#include <string>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/make_shared.hpp>
#include <boost/system/error_code.hpp>

namespace v3d::editor::document {

Document::Document(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::filesystem::path& project) :
    logger_(logger),
    scene_(boost::make_shared<scene::Scene>()),
    history_(boost::make_shared<command::Stack>()),
    project_(boost::make_shared<store::Project>(logger, project)),
    sinceAutosave_(0) {
}

const boost::shared_ptr<scene::Scene>& Document::scene() const noexcept {
    return scene_;
}

const boost::shared_ptr<command::Stack>& Document::history() const noexcept {
    return history_;
}

const boost::shared_ptr<store::Project>& Document::project() const noexcept {
    return project_;
}

bool Document::dirty() const noexcept {
    return history_->dirty();
}

Outcome Document::create(const boost::shared_ptr<scene::item::Item>& item, const std::string& name) {
    if (item->name().empty()) {
        item->name(titled(name));
    }
    // the command does the creating, so making an item and redoing one run the same code
    const boost::shared_ptr<command::Create> command = boost::make_shared<command::Create>(scene_, item, name);
    command->redo();
    history_->push(command);
    return { .done = true, .message = "created a " + name + " - " + items(scene_->count()), .undoable = true };
}

Outcome Document::createPrimitive(Primitive primitive) {
    switch (primitive) {
        case Primitive::Cube:
            return create(boost::make_shared<scene::item::Mesh>(v3d::brep::cube()), "cube");
        case Primitive::Plane:
            return create(boost::make_shared<scene::item::Mesh>(v3d::brep::plane()), "plane");
        case Primitive::Cylinder:
            return create(boost::make_shared<scene::item::Mesh>(v3d::brep::cylinder()), "cylinder");
        case Primitive::Cone:
            return create(boost::make_shared<scene::item::Mesh>(v3d::brep::cone()), "cone");
    }
    return {};
}

Outcome Document::createCamera(const v3d::type::camera::Profile& at, scene::item::Camera::Projection projection) {
    const char* name = projection == scene::item::Camera::Projection::Orthographic ? "orthographic camera" : "perspective camera";
    return create(create_camera(at, projection), name);
}

Outcome Document::createLight(const v3d::type::camera::Profile& at, scene::item::Light::Type type) {
    const char* name = "point light";
    if (type == scene::item::Light::Type::Distant) {
        name = "distant light";
    } else if (type == scene::item::Light::Type::Spot) {
        name = "spot light";
    }
    return create(create_light(at, type), name);
}

Outcome Document::createCurve(scene::item::CurveType type) {
    const char* name = "poly curve";
    if (type == scene::item::CurveType::Bezier) {
        name = "Bezier curve";
    } else if (type == scene::item::CurveType::CatmullRom) {
        name = "Catmull-Rom curve";
    } else if (type == scene::item::CurveType::Nurbs) {
        name = "NURBS curve";
    }
    return create(create_curve(type), name);
}

Outcome Document::group(const scene::Items& items) {
    const boost::shared_ptr<command::Group> command = boost::make_shared<command::Group>(scene_, items);
    if (command->empty()) {
        return { .done = false, .message = "a group gathers items, and none was given" };
    }
    command->redo();
    history_->push(command);
    return { .done = true, .message = "grouped " + Document::items(items.size()) + " - " + Document::items(scene_->count()), .undoable = true };
}

Outcome Document::hide(const scene::Items& items) {
    std::vector<command::Show::Change> changes;
    for (const boost::shared_ptr<scene::item::Item>& item : items) {
        changes.push_back({ .item = item, .shown = { .views = false, .renders = item->shown().renders } });
    }
    const boost::shared_ptr<command::Show> command = boost::make_shared<command::Show>(scene_, changes, "hide");
    if (command->empty()) {
        return { .done = false, .message = "hide: nothing given is shown" };
    }
    command->redo();
    history_->push(command);
    return { .done = true, .message = "hid " + Document::items(items.size()), .undoable = true };
}

Outcome Document::show(const scene::Items& items) {
    std::vector<command::Show::Change> changes;
    for (const boost::shared_ptr<scene::item::Item>& item : items) {
        changes.push_back({ .item = item, .shown = { .views = true, .renders = item->shown().renders } });
    }
    const boost::shared_ptr<command::Show> command = boost::make_shared<command::Show>(scene_, changes, "show");
    if (command->empty()) {
        return { .done = false, .message = "show: nothing given is hidden" };
    }
    command->redo();
    history_->push(command);
    return { .done = true, .message = "showed " + Document::items(items.size()), .undoable = true };
}

Outcome Document::showAll() {
    std::vector<command::Show::Change> changes;
    for (const boost::shared_ptr<scene::item::Item>& item : scene_->items()) {
        changes.push_back({ .item = item, .shown = { .views = true, .renders = item->shown().renders } });
    }
    const boost::shared_ptr<command::Show> command = boost::make_shared<command::Show>(scene_, changes, "show all");
    if (command->empty()) {
        return { .done = false, .message = "show all: nothing is hidden" };
    }
    command->redo();
    history_->push(command);
    return { .done = true, .message = "showed every item", .undoable = true };
}

Outcome Document::isolate(const scene::Items& items) {
    if (items.empty()) {
        return { .done = false, .message = "isolate: an isolation keeps items shown, and none was given" };
    }
    // the items and what is above them are shown, what is beneath them is left alone, and the
    // rest is hidden
    std::set<unsigned int> shown;
    std::set<unsigned int> beneath;
    for (const boost::shared_ptr<scene::item::Item>& item : items) {
        shown.insert(item->id());
        for (const boost::shared_ptr<scene::item::Item>& above : scene_->ancestors(item->id())) {
            shown.insert(above->id());
        }
    }
    for (const boost::shared_ptr<scene::item::Item>& item : scene_->items()) {
        const scene::Items above = scene_->ancestors(item->id());
        const auto given = [&items](const boost::shared_ptr<scene::item::Item>& each) { return std::ranges::find(items, each) != items.end(); };
        if (std::ranges::any_of(above, given)) {
            beneath.insert(item->id());
        }
    }
    std::vector<command::Show::Change> changes;
    for (const boost::shared_ptr<scene::item::Item>& item : scene_->items()) {
        if (!beneath.contains(item->id()) || shown.contains(item->id())) {
            changes.push_back({ .item = item, .shown = { .views = shown.contains(item->id()), .renders = item->shown().renders } });
        }
    }
    const boost::shared_ptr<command::Show> command = boost::make_shared<command::Show>(scene_, changes, "isolate");
    if (command->empty()) {
        return { .done = false, .message = "isolate: only those items are shown already" };
    }
    command->redo();
    history_->push(command);
    return { .done = true, .message = "isolated " + Document::items(items.size()), .undoable = true };
}

Outcome Document::importScene(const boost::filesystem::path& file) {
    const std::string name = file.filename().string();
    std::string extension = file.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
        [](unsigned char letter) { return static_cast<char>(std::tolower(letter)); });
    v3d::interchange::Imported read;
    if (extension == ".gltf" || extension == ".glb") {
        read = v3d::interchange::readGltf(file.string());
    } else if (extension == ".usd" || extension == ".usda" || extension == ".usdc" || extension == ".usdz") {
        read = v3d::interchange::usd::read(file.string());
    } else {
        return { .done = false, .message = "import: " + name + " is neither glTF nor USD" };
    }
    if (!read.scene) {
        return { .done = false, .message = "import: " + read.problem };
    }
    const scene::Import imported = scene::importScene(*read.scene, { .rate = scene_->timeline().rate, .metres = scene::metres(scene_->length()) });
    for (const std::string& note : read.notes) {
        logger_->get()->warn("import {}: {}", name, note);
    }
    for (const std::string& note : imported.notes) {
        logger_->get()->warn("import {}: {}", name, note);
    }
    if (imported.items.empty()) {
        return { .done = false, .message = "import: " + name + " holds nothing to import" };
    }
    const boost::shared_ptr<command::Import> command = boost::make_shared<command::Import>(scene_, imported, name);
    command->redo();
    history_->push(command);
    std::string message = "imported " + items(imported.items.size()) + " from " + name;
    const std::size_t left = read.notes.size() + imported.notes.size();
    if (left > 0) {
        message += " - " + std::to_string(left) + " things left out, which the log lists";
    }
    return { .done = true, .message = message, .undoable = true };
}

Outcome Document::key(const scene::Items& items, int frame) {
    const boost::shared_ptr<command::Key> command = boost::make_shared<command::Key>(scene_, command::Key::every(items), static_cast<float>(frame));
    if (command->empty()) {
        return { .done = false, .message = "a key is set on items, and none was given" };
    }
    command->redo();
    history_->push(command);
    return { .done = true, .message = "set a key at frame " + std::to_string(frame), .undoable = true };
}

Outcome Document::autoKey(bool on) {
    if (scene_->timeline().autoKey == on) {
        return { .done = false, .message = on ? "auto key is already on" : "auto key is already off" };
    }
    scene::Timeline wanted = scene_->timeline();
    wanted.autoKey = on;
    const boost::shared_ptr<command::Timeline> command = boost::make_shared<command::Timeline>(scene_, wanted, on ? "auto key on" : "auto key off");
    command->redo();
    history_->push(command);
    return { .done = true, .message = on ? "turned auto key on" : "turned auto key off", .undoable = true };
}

Outcome Document::timing(float rate, int start, int end) {
    scene::Timeline wanted = scene_->timeline();
    if (wanted.rate == rate && wanted.start == start && wanted.end == end) {
        return { .done = false, .message = "the frame rate and range are unchanged" };
    }
    wanted.rate = rate;
    wanted.start = start;
    wanted.end = end;
    wanted.frame = start;
    if (!wanted.valid()) {
        return { .done = false, .message = "a frame rate is a positive number, and a range runs forwards" };
    }
    const boost::shared_ptr<command::Timeline> command = boost::make_shared<command::Timeline>(scene_, wanted, "frame range");
    command->redo();
    history_->push(command);
    return { .done = true, .message = "set frames " + std::to_string(start) + " to " + std::to_string(end), .undoable = true };
}

Outcome Document::moveKeys(const std::vector<scene::FrameKey>& keys, int offset) {
    if (offset == 0) {
        return { .done = false, .message = "the keys are already there" };
    }
    const boost::shared_ptr<command::Keys> command = command::Keys::shift(scene_, keys, static_cast<float>(offset));
    if (command->empty()) {
        return { .done = false, .message = "a key is moved when one is chosen, and none was" };
    }
    command->redo();
    history_->push(command);
    const int frames = offset < 0 ? -offset : offset;
    return { .done = true, .message = "moved " + Document::keys(command->count()) + " " + std::to_string(frames) +
        (frames == 1 ? " frame " : " frames ") + (offset < 0 ? "earlier" : "later"), .undoable = true };
}

Outcome Document::deleteKeys(const std::vector<scene::FrameKey>& keys) {
    const boost::shared_ptr<command::Keys> command = command::Keys::remove(scene_, keys);
    if (command->empty()) {
        return { .done = false, .message = "a key is deleted when one is chosen, and none was" };
    }
    command->redo();
    history_->push(command);
    return { .done = true, .message = "deleted " + Document::keys(command->count()), .undoable = true };
}

Outcome Document::reshape(unsigned int item, const std::string& path, const v3d::animation::Curve& curve) {
    const boost::shared_ptr<command::Keys> command = command::Keys::replace(scene_, item, path, curve);
    if (command->empty()) {
        return { .done = false, .message = "the curve " + path + " is already that shape, or there is no such curve" };
    }
    command->redo();
    history_->push(command);
    return { .done = true, .message = "edited " + path, .undoable = true };
}

Outcome Document::settings(const scene::RenderSettings& settings) {
    if (settings == scene_->renderSettings()) {
        return { .done = false, .message = "the render settings are unchanged" };
    }
    const boost::shared_ptr<command::Settings> command = boost::make_shared<command::Settings>(scene_, settings);
    command->redo();
    history_->push(command);
    return { .done = true, .message = "changed the render settings", .undoable = true };
}

Outcome Document::splitEdges() {
    return record(history_.get(), command::splitEdges(scene_),
        {.done = "split the selected edges", .refused = "split edge: no edge is selected"});
}

Outcome Document::splitFace() {
    std::string problem;
    const boost::shared_ptr<command::Topology> command = command::splitFace(scene_, &problem);
    return record(history_.get(), command, {.done = "split a face between the selected vertices", .refused = "split face: " + problem});
}

Outcome Document::mergeVertices() {
    std::string problem;
    const boost::shared_ptr<command::Topology> command = command::mergeVertices(scene_, &problem);
    return record(history_.get(), command, {.done = "merged the selected vertices", .refused = "merge vertices: " + problem});
}

Outcome Document::extrudeFaces() {
    return record(history_.get(), command::extrudeFaces(scene_),
        {.done = "extruded the selected faces", .refused = "extrude face: no face is selected"});
}

Outcome Document::insetFaces() {
    return record(history_.get(), command::insetFaces(scene_),
        {.done = "inset the selected faces", .refused = "inset face: no face is selected"});
}

Outcome Document::undo() {
    const boost::shared_ptr<command::Command> command = history_->undo();
    if (!command) {
        return { .done = false, .message = "nothing to undo" };
    }
    return { .done = true, .message = "undo " + command->name() };
}

Outcome Document::redo() {
    const boost::shared_ptr<command::Command> command = history_->redo();
    if (!command) {
        return { .done = false, .message = "nothing to redo" };
    }
    return { .done = true, .message = "redo " + command->name(), .undoable = true };
}

void Document::goTo(int frame) {
    const scene::Timeline& timeline = scene_->timeline();
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
    // a project's directory is made when the project is first saved, so an export before then
    // makes it
    boost::system::error_code made;
    boost::filesystem::create_directories(file.parent_path(), made);
    std::ofstream stream(file.string().c_str());
    if (!stream.is_open()) {
        return { .done = false, .message = "could not write " + file.string() };
    }
    writeShot(&stream, *scene_, scene_->evaluate(), from, render::RIBExportVisitor::Output{ .settings = scene_->renderSettings(), .file = picture });
    return { .done = true, .message = "exported " + items(scene_->count()) + " to " + file.string() };
}

std::string Document::items(std::size_t count) {
    return std::to_string(count) + (count == 1 ? " item" : " items");
}

std::string Document::keys(std::size_t count) {
    return std::to_string(count) + (count == 1 ? " key" : " keys");
}

std::string Document::titled(const std::string& kind) {
    std::string title = kind;
    bool start = true;
    for (char& letter : title) {
        if (start) {
            letter = static_cast<char>(std::toupper(static_cast<unsigned char>(letter)));
        }
        start = letter == ' ';
    }
    return title;
}

Outcome Document::record(command::Stack* history, const boost::shared_ptr<command::Topology>& command, const Said& said) {
    if (!command) {
        return { .done = false, .message = said.refused };
    }
    history->push(command);
    return { .done = true, .message = said.done, .undoable = true };
}

};  // namespace v3d::editor::document
