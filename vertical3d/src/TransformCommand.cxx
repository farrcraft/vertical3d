/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "TransformCommand.h"

#include <cmath>
#include <string>

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

    namespace {

        /**
         * How far apart two placements may be and still be the same one. A drag is measured
         * in world units of a scene whose primitives are one unit across.
         **/
        const float tolerance = 1e-6f;

        /**
         **/
        bool close(float a, float b) {
            return std::fabs(a - b) <= tolerance;
        }

        /**
         **/
        bool close(const glm::vec3& a, const glm::vec3& b) {
            return close(a.x, b.x) && close(a.y, b.y) && close(a.z, b.z);
        }

        /**
         **/
        bool close(const glm::quat& a, const glm::quat& b) {
            return close(a.w, b.w) && close(a.x, b.x) && close(a.y, b.y) && close(a.z, b.z);
        }

    };  // namespace

    /**
     **/
    Placement Placement::of(const v3d::dag::Transform& transform) {
        Placement placement;
        placement.translation = transform.translation();
        placement.rotation = transform.rotation();
        placement.scale = transform.scale();
        return placement;
    }

    /**
     **/
    void Placement::applyTo(v3d::dag::Transform* transform) const {
        if (transform == nullptr) {
            return;
        }
        transform->translation(translation);
        transform->rotation(rotation);
        transform->scale(scale);
    }

    /**
     **/
    bool Placement::same(const Placement& other) const {
        return close(translation, other.translation) &&
            close(rotation, other.rotation) &&
            close(scale, other.scale);
    }

    /**
     **/
    TransformCommand::TransformCommand(const boost::shared_ptr<v3d::brep::BRep>& mesh,
        const Placement& before,
        const Placement& after,
        const std::string& mode) :
        mesh_(mesh),
        before_(before),
        after_(after),
        mode_(mode) {
    }

    /**
     **/
    void TransformCommand::undo() {
        if (mesh_) {
            before_.applyTo(mesh_.get());
        }
    }

    /**
     **/
    void TransformCommand::redo() {
        if (mesh_) {
            after_.applyTo(mesh_.get());
        }
    }

    /**
     **/
    std::string TransformCommand::name() const {
        return mode_;
    }

};  // namespace v3d::editor
