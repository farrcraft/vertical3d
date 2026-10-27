/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "TransformTool.h"

#include <api/log/Logger.h>
#include <api/type/Transform.h>
#include <vertical3d/src/command/CommandStack.h>
#include <vertical3d/src/command/Placement.h>
#include <vertical3d/src/command/TransformCommand.h>
#include <vertical3d/src/command/VertexCommand.h>
#include <vertical3d/src/manipulator/Manipulator.h>
#include <vertical3d/src/manipulator/RotateManipulator.h>
#include <vertical3d/src/manipulator/ScaleManipulator.h>
#include <vertical3d/src/manipulator/Target.h>
#include <vertical3d/src/manipulator/TranslateManipulator.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/tool/SelectMask.h>
#include <vertical3d/src/view/ViewPort.h>

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

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
    last_(0.0F, 0.0F),
    mask_(SelectMask::Object) {
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
void TransformTool::deactivate(const std::string& /* name */) {
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
    const boost::shared_ptr<Manipulator> current = manipulator();
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
void TransformTool::mask(SelectMask mask) {
    if (mask == mask_) {
        return;
    }
    // what a drag holds is about to stop being what the handles move, but what it already
    // moved has happened
    commit();
    const boost::shared_ptr<Manipulator> current = manipulator();
    if (current) {
        current->active(false);
    }
    dragging_ = false;
    mask_ = mask;
}

/**
 **/
Target TransformTool::target() const {
    if (dragging_) {
        return dragged_;
    }
    if (!scene_) {
        return Target();
    }
    return Target::selected(*scene_, mask_);
}

/**
 **/
bool TransformTool::dragging() const noexcept {
    return dragging_;
}

/**
 **/
bool TransformTool::takes(const glm::vec2& position) const {
    const boost::shared_ptr<Manipulator> current = manipulator();
    if (!current || !scene_ || !view_) {
        return false;
    }
    const Target target = this->target();
    Manipulator::Axis axis = Manipulator::Axis::None;
    return !target.empty() && current->grab(target, *view_, position, &axis);
}

/**
 **/
void TransformTool::cancel() {
    dragged_ = Target();
    before_.clear();
    shapes_.clear();
    const boost::shared_ptr<Manipulator> current = manipulator();
    if (current) {
        current->active(false);
    }
    dragging_ = false;
}

/**
 **/
void TransformTool::commit() {
    Target target;
    std::swap(target, dragged_);
    std::vector<v3d::type::Transform> before;
    before.swap(before_);
    std::vector<VertexCommand::Shape> shapes;
    shapes.swap(shapes_);
    if (!commands_ || target.empty()) {
        return;
    }

    if (target.components()) {
        std::vector<VertexCommand::Change> changes;
        bool moved = false;
        for (std::size_t index = 0; index < target.parts().size() && index < shapes.size(); index++) {
            const Target::Part& part = target.parts()[index];
            const VertexCommand::Shape after = VertexCommand::Shape::of(part);
            moved = moved || !after.same(shapes[index]);
            changes.push_back({.part = part, .before = shapes[index], .after = after});
        }
        if (moved) {
            commands_->push(boost::make_shared<VertexCommand>(changes, modeName(mode_)));
        }
        return;
    }

    // every placement is recorded in full, because a turn or a scale of several items
    // moves each one's translation as well
    const Items& items = target.items();
    std::vector<TransformCommand::Change> changes;
    bool moved = false;
    for (std::size_t index = 0; index < items.size() && index < before.size(); index++) {
        const v3d::type::Transform after = items[index]->placement();
        moved = moved || !same(after, before[index]);
        changes.push_back({.item = items[index], .before = before[index], .after = after});
    }
    if (moved) {
        commands_->push(boost::make_shared<TransformCommand>(changes, modeName(mode_)));
        return;
    }
    // a drag that measured nothing still wrote each placement, from where the evaluation put
    // it, so each item is given back the placement it held
    for (const TransformCommand::Change& change : changes) {
        change.item->placement(change.before);
    }
}

/**
 **/
void TransformTool::hover(const glm::vec2& position) {
    const boost::shared_ptr<Manipulator> current = manipulator();
    if (!current) {
        return;
    }
    if (!scene_ || !view_) {
        current->active(false);
        return;
    }

    const Target target = this->target();
    Manipulator::Axis axis = Manipulator::Axis::None;
    if (target.empty() || !current->grab(target, *view_, position, &axis)) {
        current->active(false);
        return;
    }
    current->axis(axis);
    current->active(true);
}

/**
 **/
void TransformTool::motion(const glm::vec2& position) {
    const boost::shared_ptr<Manipulator> current = manipulator();
    // a drag moves the set whose handle was grabbed rather than whatever is selected
    // now, so that a selection changing mid gesture cannot leave the record naming one
    // set and the movement landing on another
    if (dragging_ && current && !dragged_.empty() && view_) {
        current->apply(&dragged_, *view_, last_, position);
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
        // the cursor is still over the handle it released, so highlight that handle again
        hover(position);
        return;
    }

    dragging_ = false;
    const boost::shared_ptr<Manipulator> current = manipulator();
    if (!current || !scene_ || !view_) {
        return;
    }

    const Target target = this->target();
    if (target.empty()) {
        return;
    }

    Manipulator::Axis axis = Manipulator::Axis::None;
    if (!current->grab(target, *view_, position, &axis)) {
        // no handle under the cursor, so the press is not this tool's
        current->active(false);
        return;
    }
    current->axis(axis);
    current->active(true);
    dragging_ = true;
    dragged_ = target;
    before_.clear();
    shapes_.clear();
    if (target.components()) {
        for (const Target::Part& part : target.parts()) {
            shapes_.push_back(VertexCommand::Shape::of(part));
        }
    } else {
        for (const boost::shared_ptr<Item>& item : target.items()) {
            before_.push_back(item->placement());
        }
    }
}

};  // namespace v3d::editor
