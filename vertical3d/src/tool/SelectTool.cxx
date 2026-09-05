/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "SelectTool.h"

#include <string>

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

    /**
     **/
    SelectTool::SelectTool(const boost::shared_ptr<Scene>& scene, const boost::shared_ptr<v3d::log::Logger>& logger) :
        scene_(scene),
        logger_(logger),
        mask_(SelectMask::Object),
        cursor_(0.0f, 0.0f) {
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
    void SelectTool::motion(const glm::vec2& position) {
        cursor_ = position;
    }

    /**
     **/
    void SelectTool::button(unsigned int button, bool pressed, const glm::vec2& position) {
        // only the primary button selects, and on the press rather than the release - a
        // release that follows a drag out of the view would pick somewhere else
        if (button != 1 || !pressed) {
            return;
        }
        cursor_ = position;
        if (!scene_ || !view_) {
            return;
        }
        apply(picker_.pick(*scene_, *view_, cursor_, mask_));
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

        if (mask_ == SelectMask::Object) {
            if (logger_ && hit.valid) {
                logger_->get()->info("selected mesh {}", hit.mesh);
            }
            selectObject(hit.valid ? hit.mesh : 0);
            return;
        }

        if (!hit.valid) {
            // a miss in a component mode clears the components and leaves the objects
            // selected: the user is working inside one and a stray click should not undo that
            scene_->deselectComponents();
            return;
        }

        selectComponent(hit);
    }

    /**
     **/
    void SelectTool::selectObject(unsigned int id) {
        scene_->deselect();
        if (id == 0) {
            return;
        }
        boost::shared_ptr<v3d::brep::BRep> mesh = scene_->mesh(id);
        if (!mesh) {
            return;
        }
        mesh->selected(true);
    }

    /**
     **/
    void SelectTool::selectComponent(const Picker::Hit& hit) {
        boost::shared_ptr<v3d::brep::BRep> mesh = scene_->mesh(hit.mesh);
        if (!mesh) {
            return;
        }

        // read before clearing: clicking the same component twice deselects it
        bool wanted = true;
        if (hit.kind == SelectMask::Vertex) {
            v3d::brep::Vertex* vertex = mesh->vertex(hit.component);
            wanted = vertex != nullptr && !vertex->selected();
        } else if (hit.kind == SelectMask::Edge) {
            v3d::brep::HalfEdge* edge = mesh->edge(hit.component);
            wanted = edge != nullptr && !edge->selected();
        } else if (hit.kind == SelectMask::Face) {
            v3d::brep::Face* face = mesh->face(hit.component);
            wanted = face != nullptr && !face->selected();
        }

        // one component of a kind at a time
        mesh->deselectComponents();

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

        if (logger_) {
            logger_->get()->info("{} {} {} of mesh {}", wanted ? "selected" : "deselected",
                selectMaskName(hit.kind), hit.component, hit.mesh);
        }
    }

};  // namespace v3d::editor
