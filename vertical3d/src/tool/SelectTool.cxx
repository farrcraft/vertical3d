/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "SelectTool.h"

#include <api/brep/BRep.h>
#include <api/brep/Face.h>
#include <api/brep/HalfEdge.h>
#include <api/brep/Vertex.h>
#include <api/log/Logger.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/view/ViewPort.h>

#include "Picker.h"
#include "SelectMask.h"

#include <cmath>
#include <optional>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

namespace {

/**
 * Whether the component a hit names is selected. A component that is not there is not.
 **/
bool componentSelected(const v3d::brep::BRep& mesh, const Picker::Hit& hit) {
    if (hit.kind == SelectMask::Vertex) {
        const v3d::brep::Vertex* vertex = mesh.vertex(hit.component);
        return vertex != nullptr && vertex->selected();
    }
    if (hit.kind == SelectMask::Edge) {
        const v3d::brep::HalfEdge* edge = mesh.edge(hit.component);
        return edge != nullptr && edge->selected();
    }
    if (hit.kind == SelectMask::Face) {
        const v3d::brep::Face* face = mesh.face(hit.component);
        return face != nullptr && face->selected();
    }
    return false;
}

/**
 * Set the flag of the component a hit names, if it is there.
 **/
void markComponent(v3d::brep::BRep* mesh, const Picker::Hit& hit, bool wanted) {
    if (hit.kind == SelectMask::Vertex) {
        v3d::brep::Vertex* vertex = mesh->vertex(hit.component);
        if (vertex != nullptr) {
            vertex->selected(wanted);
        }
    } else if (hit.kind == SelectMask::Edge) {
        v3d::brep::HalfEdge* edge = mesh->edge(hit.component);
        if (edge != nullptr) {
            edge->selected(wanted);
        }
    } else if (hit.kind == SelectMask::Face) {
        v3d::brep::Face* face = mesh->face(hit.component);
        if (face != nullptr) {
            face->selected(wanted);
        }
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

    if (mask_ == SelectMask::Object) {
        selectObject(hit.valid ? hit.mesh : 0);
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
    const boost::shared_ptr<v3d::brep::BRep> mesh = scene_->mesh(id);
    if (!mesh) {
        return;
    }
    const bool wanted = !adding_ || !mesh->selected();
    mesh->selected(wanted);
    if (logger_) {
        logger_->get()->info("{} mesh {}", wanted ? "selected" : "deselected", id);
    }
}

/**
 **/
void SelectTool::selectComponent(const Picker::Hit& hit) {
    const boost::shared_ptr<v3d::brep::BRep> mesh = scene_->mesh(hit.mesh);
    if (!mesh) {
        return;
    }

    // read before clearing: clicking the same component twice deselects it
    const bool wanted = !componentSelected(*mesh, hit);

    if (!adding_) {
        scene_->deselectComponents();
    }

    markComponent(mesh.get(), hit, wanted);

    if (logger_) {
        logger_->get()->info("{} {} {} of mesh {}", wanted ? "selected" : "deselected",
            selectMaskName(hit.kind), hit.component, hit.mesh);
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
        const boost::shared_ptr<v3d::brep::BRep> mesh = scene_->mesh(hit.mesh);
        if (!mesh) {
            continue;
        }
        if (hit.kind == SelectMask::Object) {
            mesh->selected(true);
        } else {
            markComponent(mesh.get(), hit, true);
        }
    }
    if (logger_) {
        logger_->get()->info("band selected {} of kind {}", hits.size(), selectMaskName(mask_));
    }
}

};  // namespace v3d::editor
