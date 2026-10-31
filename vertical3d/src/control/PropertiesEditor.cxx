/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "PropertiesEditor.h"

#include <api/ui/Immediate.h>
#include <vertical3d/src/command/Edit.h>
#include <vertical3d/src/command/Stack.h>
#include <vertical3d/src/control/Outliner.h>
#include <vertical3d/src/control/Text.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/Curve.h>
#include <vertical3d/src/scene/Evaluation.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Light.h>
#include <vertical3d/src/scene/Properties.h>
#include <vertical3d/src/scene/Scene.h>

#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor::control {

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
void PropertiesEditor::draw(v3d::ui::Immediate* ui, scene::Scene* scene, std::size_t editor, const glm::vec4& region) {
    const std::string id = "properties " + std::to_string(editor);
    ui->region(id, { .position = glm::vec2(region.x, region.y), .size = glm::vec2(region.z, region.w) }, 1.0F);
    ui->pushId(id);
    const scene::Items selected = scene->selection();
    // an edit whose item is no longer shown cannot end at its field, so it ends here
    if (pending_ && (selected.size() != 1 || selected.front() != pending_->item)) {
        finish();
    }
    if (selected.size() != 1) {
        ui->textDisabled(selected.empty() ? "Nothing is selected" : std::to_string(selected.size()) + " items are selected");
    } else {
        const boost::shared_ptr<scene::Item>& item = selected.front();
        ui->text(Outliner::kind(item->kind()));
        std::string name = item->name();
        if (ui->textField("Name", &name)) {
            begin(item, "rename");
            item->name(name);
        }
        if (ui->itemFinished()) {
            finish();
        }
        numbers(ui, item, scene->evaluate().pose(*item));
        settings(ui, item);
    }
    ui->popId();
    ui->endRegion();
}

/**
 **/
void PropertiesEditor::numbers(v3d::ui::Immediate* ui, const boost::shared_ptr<scene::Item>& item, const scene::Pose& shown) {
    const scene::Item::Kind kind = item->kind();
    // the angles shown are kept closest to the last ones, so a drag through a half turn does
    // not jump to the other set of angles that give the same rotation
    if (turned_ != item) {
        turned_ = item;
        angles_ = glm::vec3(0.0F);
    }
    const std::map<std::string, float> now = values(kind, shown, angles_);
    angles_ = glm::vec3(now.at("rotation.x"), now.at("rotation.y"), now.at("rotation.z"));

    std::string group;
    for (const std::string& path : properties(kind)) {
        const Field look = field(path);
        if (look.group != group) {
            group = look.group;
            ui->separator();
            ui->text(group);
        }
        ui->pushId(path);
        float value = now.at(path);
        if (ui->dragFloat(look.label, &value, look.speed)) {
            begin(item, "edit " + lower(look.group));
            // a rotation is set whole, so the angles not dragged are given as they are shown
            std::map<std::string, float> set{ { "rotation.x", angles_.x }, { "rotation.y", angles_.y }, { "rotation.z", angles_.z } };
            set[path] = value;
            command::Edit::Values wanted = command::Edit::of(*item);
            wanted.pose = posed(kind, shown, set);
            if (valid(*item, wanted)) {
                command::Edit::apply(item.get(), wanted);
            }
        }
        if (ui->itemFinished()) {
            finish();
        }
        ui->popId();
    }
}

/**
 **/
void PropertiesEditor::settings(v3d::ui::Immediate* ui, const boost::shared_ptr<scene::Item>& item) {
    command::Edit::Values wanted = command::Edit::of(*item);
    bool changed = false;
    std::string what;
    switch (item->kind()) {
        case scene::Item::Kind::Camera: {
            int chosen = wanted.pose.lens.projection == scene::Camera::Projection::Orthographic ? 1 : 0;
            ui->separator();
            if (ui->combo("Projection", &chosen, { "Perspective", "Orthographic" })) {
                wanted.pose.lens.projection = chosen == 1 ? scene::Camera::Projection::Orthographic : scene::Camera::Projection::Perspective;
                changed = true;
                what = "edit projection";
            }
            break;
        }
        case scene::Item::Kind::Light: {
            int chosen = static_cast<int>(wanted.pose.emission.type);
            ui->separator();
            if (ui->combo("Type", &chosen, { "Point", "Distant", "Spot" })) {
                wanted.pose.emission.type = static_cast<scene::Light::Type>(chosen);
                changed = true;
                what = "edit light type";
            }
            break;
        }
        case scene::Item::Kind::Curve: {
            ui->separator();
            ui->text("Render");
            changed = ui->checkbox("Renders", &wanted.style.renders);
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
        case scene::Item::Kind::Mesh:
        case scene::Item::Kind::Group:
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
void PropertiesEditor::begin(const boost::shared_ptr<scene::Item>& item, const std::string& what) {
    if (pending_ && pending_->item != item) {
        finish();
    }
    if (!pending_) {
        pending_ = Pending{ .item = item, .before = command::Edit::of(*item), .what = what };
    }
}

/**
 **/
void PropertiesEditor::finish() {
    if (!pending_) {
        return;
    }
    if (!command::Edit::same(pending_->before, command::Edit::of(*pending_->item)) && history_) {
        history_->push(boost::make_shared<command::Edit>(pending_->item, pending_->before, pending_->what));
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
bool PropertiesEditor::valid(const scene::Item& item, const command::Edit::Values& values) {
    switch (item.kind()) {
        case scene::Item::Kind::Camera:
            return values.pose.lens.valid();
        case scene::Item::Kind::Light:
            return values.pose.emission.valid();
        case scene::Item::Kind::Curve:
            return values.style.valid();
        case scene::Item::Kind::Mesh:
        case scene::Item::Kind::Group:
            break;
    }
    return true;
}

};  // namespace v3d::editor::control
