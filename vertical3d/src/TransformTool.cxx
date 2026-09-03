/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "TransformTool.h"

#include <string>

#include "RotateManipulator.h"
#include "ScaleManipulator.h"
#include "TranslateManipulator.h"

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::editor {

    namespace {

        /**
         * What a mode is called, for the log.
         **/
        const char* modeName(TransformTool::Mode mode) {
            switch (mode) {
            case TransformTool::Mode::Translate:
                return "translate";
            case TransformTool::Mode::Rotate:
                return "rotate";
            case TransformTool::Mode::Scale:
                return "scale";
            case TransformTool::Mode::None:
            default:
                return "select";
            }
        }

    };  // namespace

    /**
     **/
    bool transformMode(const std::string& name, TransformTool::Mode* mode) {
        TransformTool::Mode wanted;
        if (name == "select") {
            wanted = TransformTool::Mode::None;
        } else if (name == "translate") {
            wanted = TransformTool::Mode::Translate;
        } else if (name == "rotate") {
            wanted = TransformTool::Mode::Rotate;
        } else if (name == "scale") {
            wanted = TransformTool::Mode::Scale;
        } else {
            return false;
        }
        if (mode != nullptr) {
            *mode = wanted;
        }
        return true;
    }

    /**
     **/
    TransformTool::TransformTool(const boost::shared_ptr<Scene>& scene,
        const boost::shared_ptr<v3d::log::Logger>& logger) :
        scene_(scene),
        logger_(logger),
        translate_(boost::make_shared<TranslateManipulator>()),
        rotate_(boost::make_shared<RotateManipulator>()),
        scale_(boost::make_shared<ScaleManipulator>()),
        mode_(Mode::None),
        dragging_(false),
        last_(0.0f, 0.0f) {
    }

    /**
     **/
    void TransformTool::activate(const std::string& name) {
        Mode wanted = mode_;
        if (transformMode(name, &wanted)) {
            mode(wanted);
        }
    }

    /**
     **/
    void TransformTool::deactivate(const std::string& name) {
        // the modes latch rather than being held, so there is nothing to undo when the key
        // that chose one comes up
    }

    /**
     **/
    void TransformTool::commands(const boost::shared_ptr<CommandStack>& commands) {
        commands_ = commands;
    }

    /**
     **/
    void TransformTool::view(const boost::shared_ptr<ViewPort>& view) {
        view_ = view;
    }

    /**
     **/
    boost::shared_ptr<ViewPort> TransformTool::view() const {
        return view_;
    }

    /**
     **/
    TransformTool::Mode TransformTool::mode() const noexcept {
        return mode_;
    }

    /**
     **/
    void TransformTool::mode(Mode mode) {
        if (mode == mode_) {
            return;
        }
        // the handle being dragged is about to stop existing, but what it already wrote to
        // the mesh has happened
        commit();
        boost::shared_ptr<Manipulator> current = manipulator();
        if (current) {
            current->active(false);
        }
        dragging_ = false;
        mode_ = mode;
        if (logger_) {
            logger_->get()->info("transform tool is {}", modeName(mode_));
        }
    }

    /**
     **/
    boost::shared_ptr<Manipulator> TransformTool::manipulator() const {
        switch (mode_) {
        case Mode::Translate:
            return translate_;
        case Mode::Rotate:
            return rotate_;
        case Mode::Scale:
            return scale_;
        case Mode::None:
        default:
            return boost::shared_ptr<Manipulator>();
        }
    }

    /**
     **/
    bool TransformTool::dragging() const noexcept {
        return dragging_;
    }

    /**
     **/
    void TransformTool::cancel() {
        dragged_.reset();
        boost::shared_ptr<Manipulator> current = manipulator();
        if (current) {
            current->active(false);
        }
        dragging_ = false;
    }

    /**
     **/
    void TransformTool::commit() {
        boost::shared_ptr<v3d::brep::BRep> mesh = dragged_;
        dragged_.reset();
        if (!mesh || !commands_) {
            return;
        }
        const Placement after = Placement::of(*mesh);
        if (after.same(before_)) {
            return;
        }
        commands_->push(boost::make_shared<TransformCommand>(mesh, before_, after, modeName(mode_)));
    }

    /**
     **/
    void TransformTool::hover(const glm::vec2& position) {
        boost::shared_ptr<Manipulator> current = manipulator();
        if (!current) {
            return;
        }
        if (!scene_ || !view_) {
            current->active(false);
            return;
        }

        boost::shared_ptr<v3d::brep::BRep> mesh = scene_->selection();
        Manipulator::Axis axis = Manipulator::Axis::None;
        if (!mesh || !current->grab(mesh, *view_, position, &axis)) {
            current->active(false);
            return;
        }
        current->axis(axis);
        current->active(true);
    }

    /**
     **/
    void TransformTool::motion(const glm::vec2& position) {
        boost::shared_ptr<Manipulator> current = manipulator();
        // a drag moves the mesh whose handle was grabbed rather than whatever is selected
        // now, so that a selection changing mid gesture cannot leave the record naming one
        // mesh and the movement landing on another
        if (dragging_ && current && dragged_ && view_) {
            current->apply(dragged_, *view_, last_, position);
        } else {
            hover(position);
        }
        last_ = position;
    }

    /**
     **/
    void TransformTool::button(unsigned int button, bool pressed, const glm::vec2& position) {
        // only the primary button drags a handle
        if (button != 1) {
            return;
        }

        last_ = position;

        if (!pressed) {
            dragging_ = false;
            commit();
            // the cursor is still over whatever it let go of, and the handle should say so
            hover(position);
            return;
        }

        dragging_ = false;
        boost::shared_ptr<Manipulator> current = manipulator();
        if (!current || !scene_ || !view_) {
            return;
        }

        boost::shared_ptr<v3d::brep::BRep> mesh = scene_->selection();
        if (!mesh) {
            return;
        }

        Manipulator::Axis axis = Manipulator::Axis::None;
        if (!current->grab(mesh, *view_, position, &axis)) {
            // no handle under the cursor, so the press is not this tool's - the controller
            // passes it on to the one that selects
            current->active(false);
            return;
        }
        current->axis(axis);
        current->active(true);
        dragging_ = true;
        dragged_ = mesh;
        before_ = Placement::of(*mesh);
    }

};  // namespace v3d::editor
