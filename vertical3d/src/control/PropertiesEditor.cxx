/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "PropertiesEditor.h"

#include <api/brep/BRep.h>
#include <api/ui/Image.h>
#include <api/ui/Immediate.h>
#include <vertical3d/src/command/Edit.h>
#include <vertical3d/src/command/Keyed.h>
#include <vertical3d/src/command/Parent.h>
#include <vertical3d/src/command/Show.h>
#include <vertical3d/src/command/Stack.h>
#include <vertical3d/src/command/Units.h>
#include <vertical3d/src/control/Outliner.h>
#include <vertical3d/src/control/Text.h>
#include <vertical3d/src/scene/item/Camera.h>
#include <vertical3d/src/scene/item/Curve.h>
#include <vertical3d/src/scene/Evaluation.h>
#include <vertical3d/src/scene/item/Item.h>
#include <vertical3d/src/scene/item/Light.h>
#include <vertical3d/src/scene/item/Mesh.h>
#include <vertical3d/src/scene/Properties.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Units.h>
#include <vertical3d/src/view/ViewOverlay.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor::control {

namespace {

/**
 * The colours a row of x, y and z leads each field's letter with, the axes' as the views draw them.
 **/
constexpr std::array<glm::vec4, 3> AXES = { view::OverlayLook::X_AXIS, view::OverlayLook::Y_AXIS, view::OverlayLook::Z_AXIS };

};  // namespace

/**
 **/
PropertiesEditor::Field PropertiesEditor::field(const std::string& path) {
    const auto found = fields().find(path);
    if (found == fields().end()) {
        return { .group = std::string(), .label = path, .speed = DEFAULT_SPEED };
    }
    return found->second;
}

/**
 **/
PropertiesEditor::PropertiesEditor(const boost::shared_ptr<command::Stack>& history) :
    history_(history) {
}

/**
 **/
void PropertiesEditor::draw(v3d::ui::Immediate* ui, const boost::shared_ptr<scene::Scene>& scene, std::size_t editor, const glm::vec4& region) {
    scene_ = scene;
    editor_ = editor;
    const std::string id = "properties " + std::to_string(editor);
    ui->region(id, { .position = glm::vec2(region.x, region.y), .size = glm::vec2(region.z, region.w) }, 1.0F);
    ui->pushId(id);
    const scene::Items selected = scene->selection();
    // an edit whose item is no longer shown cannot end at its field, so it ends here
    if (pending_ && (selected.size() != 1 || selected.front() != pending_->item)) {
        finish();
    }
    if (selected.empty()) {
        sceneSettings(ui, scene);
    } else if (selected.size() > 1) {
        ui->textDisabled(std::to_string(selected.size()) + " items are selected");
    } else {
        const boost::shared_ptr<scene::item::Item>& item = selected.front();
        ui->tabBar("tabs");
        if (ui->tab("Item")) {
            itemTab(ui, scene, item);
        }
        if (item->kind() == scene::item::Item::Kind::Mesh && ui->tab("Mesh")) {
            meshCounts(ui, *item);
        }
        if (ui->tab("Keys")) {
            keys(ui, *scene, *item);
        }
        ui->endTabBar();
    }
    ui->popId();
    ui->endRegion();
}

/**
 **/
void PropertiesEditor::itemTab(v3d::ui::Immediate* ui, const boost::shared_ptr<scene::Scene>& scene, const boost::shared_ptr<scene::item::Item>& item) {
    const scene::item::Item::Kind kind = item->kind();
    const scene::Pose shown = scene->evaluate().pose(*item);
    std::vector<std::string> placed;
    std::vector<std::string> own;
    for (const std::string& path : scene::properties(kind)) {
        const bool placing = path.starts_with("translation.") || path.starts_with("rotation.") || path.starts_with("scale.");
        (placing ? placed : own).push_back(path);
    }
    if (heading(ui, "Transform")) {
        numbers(ui, item, shown, scene->length(), placed);
    }
    // what a kind has of its own is headed by the kind's name. A mesh and a group have nothing
    if (kind != scene::item::Item::Kind::Mesh && kind != scene::item::Item::Kind::Group) {
        ui->separator();
        if (heading(ui, scene::item::name(kind))) {
            numbers(ui, item, shown, scene->length(), own);
            settings(ui, item);
        }
    }
    ui->separator();
    if (heading(ui, "Item")) {
        itemSettings(ui, scene, item);
    }
}

/**
 **/
bool PropertiesEditor::heading(v3d::ui::Immediate* ui, const std::string& title) {
    std::set<std::string>& folded = folded_[editor_];
    bool open = !folded.contains(title);
    const bool shown = ui->heading(title, &open);
    if (open) {
        folded.erase(title);
    } else {
        folded.insert(title);
    }
    return shown;
}

/**
 **/
void PropertiesEditor::itemSettings(v3d::ui::Immediate* ui, const boost::shared_ptr<scene::Scene>& scene, const boost::shared_ptr<scene::item::Item>& item) {
    std::string name = item->name();
    if (ui->textField("Name", &name, v3d::ui::Immediate::FieldLook{ .icon = v3d::ui::Image(), .labelled = true, .limit = 0 })) {
        begin(item, "rename");
        item->name(name);
    }
    if (ui->itemFinished()) {
        finish();
    }

    // an item can go beneath any other but itself and what is beneath it
    const boost::shared_ptr<scene::item::Item> current = scene->parent(item->id());
    scene::Items offered{ nullptr };
    std::vector<std::string> names{ "None" };
    int chosen = 0;
    for (const boost::shared_ptr<scene::item::Item>& other : scene->items()) {
        const scene::Items above = scene->ancestors(other->id());
        if (other == item || std::ranges::find(above, item) != above.end()) {
            continue;
        }
        if (other == current) {
            chosen = static_cast<int>(offered.size());
        }
        offered.push_back(other);
        names.push_back(Outliner::label(*other));
    }
    if (ui->combo("Parent", &chosen, names) && chosen >= 0 && static_cast<std::size_t>(chosen) < offered.size()) {
        finish();
        const boost::shared_ptr<command::Parent> change = boost::make_shared<command::Parent>(scene,
            command::Parent::Move{ .item = item, .parent = offered[static_cast<std::size_t>(chosen)] });
        if (!change->empty()) {
            change->redo();
            if (history_) {
                history_->push(change);
            }
        }
    }

    scene::item::Item::Shown shown = item->shown();
    if (ui->checkbox("Shown in renders", &shown.renders)) {
        finish();
        const boost::shared_ptr<command::Show> change = boost::make_shared<command::Show>(scene,
            std::vector<command::Show::Change>{ { .item = item, .shown = shown } }, "shown in renders");
        if (!change->empty()) {
            change->redo();
            if (history_) {
                history_->push(change);
            }
        }
    }
}

/**
 **/
void PropertiesEditor::numbers(v3d::ui::Immediate* ui, const boost::shared_ptr<scene::item::Item>& item, const scene::Pose& shown, scene::Length length,
    const std::vector<std::string>& paths) {
    const scene::item::Item::Kind kind = item->kind();
    // the angles shown are kept closest to the last ones, so a drag through a half turn does
    // not jump to the other set of angles that give the same rotation
    if (turned_ != item) {
        turned_ = item;
        angles_ = glm::vec3(0.0F);
    }
    const std::map<std::string, float> now = values(kind, shown, angles_);
    angles_ = glm::vec3(now.at("rotation.x"), now.at("rotation.y"), now.at("rotation.z"));

    for (std::size_t at = 0; at < paths.size();) {
        const std::string group = field(paths[at]).group;
        std::size_t end = at;
        while (end < paths.size() && field(paths[end]).group == group) {
            end++;
        }
        ui->text(group);
        // a group along x, y and z shares one row, each field a third of it, edged in its axis's
        // colour
        const bool axes = end - at == AXES.size() && paths[at].ends_with(".x") && paths[at + 1].ends_with(".y") &&
            paths[at + 2].ends_with(".z");
        const float third = (ui->rowWidth() - (2.0F * ui->dressing().spacing)) / 3.0F;
        for (std::size_t index = at; index < end; index++) {
            v3d::ui::Immediate::NumberLook look{ .labelColour = std::nullopt, .unit = unit(paths[index], length) };
            if (axes) {
                if (index > at) {
                    ui->sameLine();
                }
                ui->nextItemWidth(third);
                look.labelColour = AXES[index - at];
                look.edge = AXES[index - at];
            }
            number(ui, item, shown, now, paths[index], look);
        }
        at = end;
    }
}

/**
 **/
void PropertiesEditor::number(v3d::ui::Immediate* ui, const boost::shared_ptr<scene::item::Item>& item, const scene::Pose& shown,
    const std::map<std::string, float>& now, const std::string& path, const v3d::ui::Immediate::NumberLook& look) {
    const Field named = field(path);
    ui->pushId(path);
    float value = now.at(path);
    if (ui->dragFloat(named.label, &value, named.speed, look)) {
        begin(item, "edit " + lower(named.group));
        // a rotation is set whole, so the angles not dragged are given as they are shown
        std::map<std::string, float> set{ { "rotation.x", angles_.x }, { "rotation.y", angles_.y }, { "rotation.z", angles_.z } };
        set[path] = value;
        command::Edit::Values wanted = command::Edit::of(*item);
        wanted.pose = posed(item->kind(), shown, set);
        if (valid(*item, wanted)) {
            command::Edit::apply(item.get(), wanted);
        }
    }
    if (ui->itemFinished()) {
        finish();
    }
    ui->popId();
}

/**
 **/
void PropertiesEditor::sceneSettings(v3d::ui::Immediate* ui, const boost::shared_ptr<scene::Scene>& scene) {
    ui->text("Scene");
    std::vector<std::string> names;
    int chosen = 0;
    for (std::size_t index = 0; index < scene::LENGTHS.size(); index++) {
        names.emplace_back(scene::name(scene::LENGTHS[index]));
        if (scene::LENGTHS[index] == scene->length()) {
            chosen = static_cast<int>(index);
        }
    }
    if (ui->combo("Unit", &chosen, names) && chosen >= 0 && static_cast<std::size_t>(chosen) < scene::LENGTHS.size()) {
        const boost::shared_ptr<command::Units> change = boost::make_shared<command::Units>(scene, scene::LENGTHS[static_cast<std::size_t>(chosen)]);
        change->redo();
        if (history_) {
            history_->push(change);
        }
    }
}

/**
 **/
void PropertiesEditor::meshCounts(v3d::ui::Immediate* ui, const scene::item::Item& item) {
    const auto* mesh = dynamic_cast<const scene::item::Mesh*>(&item);
    if (mesh == nullptr || !mesh->geometry()) {
        return;
    }
    ui->text("Vertices " + std::to_string(mesh->geometry()->vertexCount()));
    ui->text("Faces " + std::to_string(mesh->geometry()->faceCount()));
}

/**
 **/
void PropertiesEditor::keys(v3d::ui::Immediate* ui, const scene::Scene& scene, const scene::item::Item& item) {
    const scene::Curves curves = scene.curves(item.id());
    if (curves.empty()) {
        ui->textDisabled("No keys");
        return;
    }
    for (const auto& [path, curve] : curves) {
        const Field named = field(path);
        const std::size_t count = curve.keys().size();
        ui->text(named.group + " " + named.label + "  " + std::to_string(count) + (count == 1 ? " key" : " keys"));
    }
}

/**
 **/
std::string PropertiesEditor::unit(const std::string& path, scene::Length length) {
    if (path.starts_with("translation.") || path == "lens.near" || path == "lens.far" || path == "lens.extent") {
        return " " + std::string(scene::symbol(length));
    }
    if (path.starts_with("rotation.") || path == "lens.fov") {
        return "°";
    }
    if (path == "emission.cone" || path == "emission.delta") {
        return " rad";
    }
    return std::string();
}

/**
 **/
void PropertiesEditor::settings(v3d::ui::Immediate* ui, const boost::shared_ptr<scene::item::Item>& item) {
    command::Edit::Values wanted = command::Edit::of(*item);
    bool changed = false;
    std::string what;
    switch (item->kind()) {
        case scene::item::Item::Kind::Camera: {
            int chosen = wanted.pose.lens.projection == scene::item::Camera::Projection::Orthographic ? 1 : 0;
            if (ui->combo("Projection", &chosen, { "Perspective", "Orthographic" })) {
                wanted.pose.lens.projection = chosen == 1 ? scene::item::Camera::Projection::Orthographic : scene::item::Camera::Projection::Perspective;
                changed = true;
                what = "edit projection";
            }
            break;
        }
        case scene::item::Item::Kind::Light: {
            int chosen = static_cast<int>(wanted.pose.emission.type);
            if (ui->combo("Type", &chosen, { "Point", "Distant", "Spot" })) {
                wanted.pose.emission.type = static_cast<scene::item::Light::Type>(chosen);
                changed = true;
                what = "edit light type";
            }
            break;
        }
        case scene::item::Item::Kind::Curve: {
            ui->text("Render");
            changed = ui->checkbox("Ribbons", &wanted.style.renders);
            what = "edit render";
            if (ui->dragFloat("Width", &wanted.style.width, WIDTH_SPEED) && valid(*item, wanted)) {
                begin(item, "edit width");
                command::Edit::apply(item.get(), wanted);
            }
            if (ui->itemFinished()) {
                finish();
            }
            break;
        }
        case scene::item::Item::Kind::Mesh:
        case scene::item::Item::Kind::Group:
            break;
    }
    // a choice is a whole edit, which ends as it is made
    if (changed && valid(*item, wanted)) {
        begin(item, what);
        command::Edit::apply(item.get(), wanted);
        finish();
    }
}

/**
 **/
void PropertiesEditor::begin(const boost::shared_ptr<scene::item::Item>& item, const std::string& what) {
    if (pending_ && pending_->item != item) {
        finish();
    }
    if (!pending_) {
        pending_ = Pending{ .scene = scene_, .item = item, .before = command::Edit::of(*item), .what = what };
    }
}

/**
 **/
void PropertiesEditor::finish() {
    if (!pending_) {
        return;
    }
    // a name is free to match another while it is typed, and is made unique when the edit ends
    if (pending_->scene) {
        const std::string name = pending_->scene->available(pending_->item->name(), *pending_->item);
        if (name != pending_->item->name()) {
            pending_->item->name(name);
        }
    }
    if (!command::Edit::same(pending_->before, command::Edit::of(*pending_->item)) && history_) {
        const command::Keyed::Change change{ .item = pending_->item, .before = pending_->before.pose, .after = scene::pose(*pending_->item) };
        history_->push(command::Keyed::record(pending_->scene, boost::make_shared<command::Edit>(pending_->item, pending_->before, pending_->what), { change }));
    }
    pending_.reset();
}

/**
 **/
const std::map<std::string, PropertiesEditor::Field>& PropertiesEditor::fields() {
    static const std::map<std::string, PropertiesEditor::Field> each{
        { "translation.x", { "Translation", "X", 0.01F } },
        { "translation.y", { "Translation", "Y", 0.01F } },
        { "translation.z", { "Translation", "Z", 0.01F } },
        { "rotation.x", { "Rotation", "X", 0.5F } },
        { "rotation.y", { "Rotation", "Y", 0.5F } },
        { "rotation.z", { "Rotation", "Z", 0.5F } },
        { "scale.x", { "Scale", "X", 0.01F } },
        { "scale.y", { "Scale", "Y", 0.01F } },
        { "scale.z", { "Scale", "Z", 0.01F } },
        { "lens.fov", { "Lens", "Field of View", 0.1F } },
        { "lens.extent", { "Lens", "Extent", 0.01F } },
        { "lens.near", { "Lens", "Near", 0.01F } },
        { "lens.far", { "Lens", "Far", 1.0F } },
        { "emission.intensity", { "Emission", "Intensity", 0.05F } },
        { "emission.red", { "Emission", "Red", 0.005F } },
        { "emission.green", { "Emission", "Green", 0.005F } },
        { "emission.blue", { "Emission", "Blue", 0.005F } },
        { "emission.cone", { "Emission", "Cone", 0.005F } },
        { "emission.delta", { "Emission", "Soft Edge", 0.005F } },
    };
    return each;
}

/**
 **/
bool PropertiesEditor::valid(const scene::item::Item& item, const command::Edit::Values& values) {
    switch (item.kind()) {
        case scene::item::Item::Kind::Camera:
            return values.pose.lens.valid();
        case scene::item::Item::Kind::Light:
            return values.pose.emission.valid();
        case scene::item::Item::Kind::Curve:
            return values.style.valid();
        case scene::item::Item::Kind::Mesh:
        case scene::item::Item::Kind::Group:
            break;
    }
    return true;
}

};  // namespace v3d::editor::control
