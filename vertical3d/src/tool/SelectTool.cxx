/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "SelectTool.h"

#include <api/brep/BRep.h>
#include <api/brep/Index.h>
#include <api/curve/Curves.h>
#include <api/log/Logger.h>
#include <vertical3d/src/scene/Components.h>
#include <vertical3d/src/scene/Curve.h>
#include <vertical3d/src/scene/CurveParts.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/selection/SelectMask.h>
#include <vertical3d/src/view/ViewPort.h>

#include "Picker.h"

#include <cmath>
#include <optional>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

namespace {

/**
 * Whether the mesh holds the component a hit names.
 **/
bool holds(const v3d::brep::BRep& mesh, Component kind, v3d::brep::Index index) {
    switch (kind) {
        case Component::Vertex:
            return index < mesh.vertexCount();
        case Component::Edge:
            return index < mesh.edgeCount();
        case Component::Face:
            return index < mesh.faceCount();
        case Component::Handle:
            break;
    }
    return false;
}

/**
 * Whether the curves hold the component a hit names.
 **/
bool holds(const v3d::curve::Curves& curves, Component kind, v3d::brep::Index index) {
    switch (kind) {
        case Component::Vertex:
            return pointAt(curves, index).has_value();
        case Component::Handle:
            return handleAt(curves, index).has_value();
        case Component::Edge:
        case Component::Face:
            break;
    }
    return false;
}

/**
 * Whether the item holds the component a hit names. Only a mesh and a curve have components.
 **/
bool holds(const boost::shared_ptr<Item>& item, Component kind, v3d::brep::Index index) {
    if (const boost::shared_ptr<Mesh> mesh = as<Mesh>(item)) {
        return holds(*mesh->geometry(), kind, index);
    }
    if (const boost::shared_ptr<Curve> curve = as<Curve>(item)) {
        return holds(*curve->geometry(), kind, index);
    }
    return false;
}

/**
 * Whether the component a hit names is selected. A component that is not there is not.
 **/
bool componentSelected(const Components& components, const boost::shared_ptr<Item>& item, const Picker::Hit& hit) {
    const std::optional<Component> kind = componentOf(hit.kind);
    return kind && holds(item, *kind, hit.component) && components.selected(item->id(), *kind, hit.component);
}

/**
 * Select or deselect the component a hit names, if it is there.
 **/
void markComponent(Components* components, const boost::shared_ptr<Item>& item, const Picker::Hit& hit, bool wanted) {
    const std::optional<Component> kind = componentOf(hit.kind);
    if (kind && holds(item, *kind, hit.component)) {
        components->select(item->id(), *kind, hit.component, wanted);
    }
}

};  // namespace

/**
 **/
SelectTool::SelectTool(const boost::shared_ptr<Scene>& scene, const boost::shared_ptr<v3d::log::Logger>& logger) :
    scene_(scene),
    logger_(logger),
    mask_(SelectMask::Object),
    adding_(false),
    cursor_(0.0F, 0.0F),
    held_(false),
    anchor_(0.0F, 0.0F) {
}

/**
 **/
void SelectTool::activate(const std::string& name) {
    SelectMask wanted = mask_;
    if (selectMask(name, &wanted)) {
        mask(wanted);
    }
}

/**
 **/
void SelectTool::deactivate(const std::string& /* name */) {
}

/**
 **/
void SelectTool::view(const boost::shared_ptr<ViewPort>& view) {
    view_ = view;
}

/**
 **/
boost::shared_ptr<ViewPort> SelectTool::view() const {
    return view_;
}

/**
 **/
SelectMask SelectTool::mask() const noexcept {
    return mask_;
}

/**
 **/
void SelectTool::mask(SelectMask mask) {
    if (mask == mask_) {
        return;
    }
    mask_ = mask;
    if (scene_) {
        scene_->deselectComponents();
    }
    hit_ = Picker::Hit();
    if (logger_) {
        logger_->get()->info("select mask is {}", selectMaskName(mask_));
    }
}

/**
 **/
void SelectTool::adding(bool adding) noexcept {
    adding_ = adding;
}

/**
 **/
bool SelectTool::adding() const noexcept {
    return adding_;
}

/**
 **/
void SelectTool::motion(const glm::vec2& position) {
    cursor_ = position;
}

/**
 **/
void SelectTool::button(unsigned int button, bool pressed, const glm::vec2& position) {
    // only the primary button selects
    if (button != 1) {
        return;
    }
    cursor_ = position;

    // a click picks on the press, not the release. A release that follows a drag out of
    // the view would pick somewhere else. The release only ends a rubber band
    if (!pressed) {
        if (band()) {
            selectInside();
        }
        held_ = false;
        return;
    }

    held_ = false;
    if (!scene_ || !view_) {
        return;
    }
    apply(picker_.pick(*scene_, *view_, cursor_, mask_));
    held_ = !hit_.valid;
    anchor_ = cursor_;
}

/**
 **/
bool SelectTool::dragging() const noexcept {
    return held_;
}

/**
 **/
bool SelectTool::takes(const glm::vec2& /* position */) const {
    return true;
}

/**
 **/
std::optional<SelectTool::Band> SelectTool::band() const {
    if (!held_) {
        return std::nullopt;
    }
    // a shaky click is still a click, and does not select an empty band
    if (std::abs(cursor_.x - anchor_.x) <= clickTolerance && std::abs(cursor_.y - anchor_.y) <= clickTolerance) {
        return std::nullopt;
    }
    return Band{ .corner = anchor_, .opposite = cursor_ };
}

/**
 **/
const Picker::Hit& SelectTool::hit() const noexcept {
    return hit_;
}

/**
 **/
void SelectTool::apply(const Picker::Hit& hit) {
    hit_ = hit;

    if (logger_ && !hit.valid) {
        logger_->get()->info("selected nothing");
    }

    if (!hit.valid && adding_) {
        // a miss while adding has nothing to add, and the selection is kept
        return;
    }

    if (selectsItems(mask_)) {
        selectObject(hit.valid ? hit.item : 0);
        return;
    }

    if (!hit.valid) {
        // a miss in a component mode clears the components and leaves the objects
        // selected, so a stray click does not lose the objects the user is working inside
        scene_->deselectComponents();
        return;
    }

    selectComponent(hit);
}

/**
 **/
void SelectTool::selectObject(unsigned int id) {
    if (!adding_) {
        scene_->deselect();
    }
    if (id == 0) {
        return;
    }
    const boost::shared_ptr<Item> item = scene_->item(id);
    if (!item) {
        return;
    }
    const bool wanted = !adding_ || !item->selected();
    item->selected(wanted);
    if (logger_) {
        logger_->get()->info("{} item {}", wanted ? "selected" : "deselected", id);
    }
}

/**
 **/
void SelectTool::selectComponent(const Picker::Hit& hit) {
    const boost::shared_ptr<Item> item = scene_->item(hit.item);
    if (!item) {
        return;
    }

    // read before clearing: clicking the same component twice deselects it
    const bool wanted = !componentSelected(scene_->components(), item, hit);

    if (!adding_) {
        scene_->deselectComponents();
    }

    markComponent(&scene_->components(), item, hit, wanted);

    if (logger_) {
        logger_->get()->info("{} {} {} of item {}", wanted ? "selected" : "deselected",
            selectMaskName(hit.kind), hit.component, hit.item);
    }
}

/**
 **/
void SelectTool::selectInside() {
    if (!scene_ || !view_) {
        return;
    }
    const std::vector<Picker::Hit> hits = picker_.inside(*scene_, *view_, anchor_, cursor_, mask_);
    for (const Picker::Hit& hit : hits) {
        if (selectsItems(hit.kind)) {
            const boost::shared_ptr<Item> item = scene_->item(hit.item);
            if (item) {
                item->selected(true);
            }
            continue;
        }
        const boost::shared_ptr<Item> item = scene_->item(hit.item);
        if (item) {
            markComponent(&scene_->components(), item, hit, true);
        }
    }
    if (logger_) {
        logger_->get()->info("band selected {} of kind {}", hits.size(), selectMaskName(mask_));
    }
}

};  // namespace v3d::editor
