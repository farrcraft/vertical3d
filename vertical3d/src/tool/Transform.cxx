/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Transform.h"

#include <api/log/Logger.h>
#include <api/type/Transform.h>
#include <vertical3d/src/command/Keyed.h>
#include <vertical3d/src/command/Placement.h>
#include <vertical3d/src/command/Stack.h>
#include <vertical3d/src/command/Transform.h>
#include <vertical3d/src/command/Vertex.h>
#include <vertical3d/src/manipulator/Manipulator.h>
#include <vertical3d/src/manipulator/Rotate.h>
#include <vertical3d/src/manipulator/Scale.h>
#include <vertical3d/src/manipulator/Screen.h>
#include <vertical3d/src/manipulator/Target.h>
#include <vertical3d/src/manipulator/Translate.h>
#include <vertical3d/src/scene/item/Item.h>
#include <vertical3d/src/scene/Properties.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/selection/SelectMask.h>
#include <vertical3d/src/view/ViewPort.h>

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::editor::tool {

/**
 **/
const char* Transform::modeName(Transform::Mode mode) {
    switch (mode) {
    case Transform::Mode::Translate:
        return "translate";
    case Transform::Mode::Rotate:
        return "rotate";
    case Transform::Mode::Scale:
        return "scale";
    case Transform::Mode::None:
    default:
        return "select";
    }
}

/**
 **/
bool transformMode(const std::string& name, Transform::Mode* mode) {
    Transform::Mode wanted;
    if (name == "select") {
        wanted = Transform::Mode::None;
    } else if (name == "translate") {
        wanted = Transform::Mode::Translate;
    } else if (name == "rotate") {
        wanted = Transform::Mode::Rotate;
    } else if (name == "scale") {
        wanted = Transform::Mode::Scale;
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
Transform::Transform(const boost::shared_ptr<scene::Scene>& scene,
    const boost::shared_ptr<v3d::log::Logger>& logger) :
    scene_(scene),
    logger_(logger),
    translate_(boost::make_shared<manipulator::Translate>()),
    rotate_(boost::make_shared<manipulator::Rotate>()),
    scale_(boost::make_shared<manipulator::Scale>()),
    mode_(Mode::None),
    dragging_(false),
    last_(0.0F, 0.0F),
    mask_(selection::SelectMask::Object) {
}

/**
 **/
void Transform::activate(const std::string& name) {
    Mode wanted = mode_;
    if (transformMode(name, &wanted)) {
        mode(wanted);
    }
}

/**
 **/
void Transform::deactivate(const std::string& /* name */) {
    // the modes latch rather than being held, so there is nothing to undo when the key
    // that chose one comes up
}

/**
 **/
void Transform::commands(const boost::shared_ptr<command::Stack>& commands) {
    commands_ = commands;
}

/**
 **/
void Transform::view(const boost::shared_ptr<view::ViewPort>& view) {
    view_ = view;
}

/**
 **/
boost::shared_ptr<view::ViewPort> Transform::view() const {
    return view_;
}

/**
 **/
Transform::Mode Transform::mode() const noexcept {
    return mode_;
}

/**
 **/
void Transform::mode(Mode mode) {
    if (mode == mode_) {
        return;
    }
    // the handle being dragged is about to stop existing, but what it already wrote to
    // the mesh has happened
    commit();
    const boost::shared_ptr<manipulator::Manipulator> current = manipulator();
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
boost::shared_ptr<manipulator::Manipulator> Transform::manipulator() const {
    switch (mode_) {
    case Mode::Translate:
        return translate_;
    case Mode::Rotate:
        return rotate_;
    case Mode::Scale:
        return scale_;
    case Mode::None:
    default:
        return boost::shared_ptr<manipulator::Manipulator>();
    }
}

/**
 **/
void Transform::mask(selection::SelectMask mask) {
    if (mask == mask_) {
        return;
    }
    // what a drag holds is about to stop being what the handles move, but what it already
    // moved has happened
    commit();
    const boost::shared_ptr<manipulator::Manipulator> current = manipulator();
    if (current) {
        current->active(false);
    }
    dragging_ = false;
    mask_ = mask;
}

/**
 **/
manipulator::Screen Transform::screen() const {
    return manipulator::Screen{ .camera = view_->camera(), .region = view_->region() };
}

/**
 **/
manipulator::Target Transform::target() const {
    if (dragging_) {
        return dragged_;
    }
    if (!scene_) {
        return manipulator::Target();
    }
    return manipulator::Target::selected(*scene_, mask_);
}

/**
 **/
bool Transform::dragging() const noexcept {
    return dragging_;
}

/**
 **/
bool Transform::takes(const glm::vec2& position) const {
    const boost::shared_ptr<manipulator::Manipulator> current = manipulator();
    if (!current || !scene_ || !view_) {
        return false;
    }
    const manipulator::Target target = this->target();
    manipulator::Manipulator::Axis axis = manipulator::Manipulator::Axis::None;
    return !target.empty() && current->grab(target, screen(), position, &axis);
}

/**
 **/
void Transform::cancel() {
    dragged_ = manipulator::Target();
    before_.clear();
    shapes_.clear();
    outlines_.clear();
    const boost::shared_ptr<manipulator::Manipulator> current = manipulator();
    if (current) {
        current->active(false);
    }
    dragging_ = false;
}

/**
 **/
void Transform::commit() {
    manipulator::Target target;
    std::swap(target, dragged_);
    std::vector<v3d::type::Transform> before;
    before.swap(before_);
    std::vector<command::Vertex::Shape> shapes;
    shapes.swap(shapes_);
    std::vector<command::Vertex::Outline> outlines;
    outlines.swap(outlines_);
    if (!commands_ || target.empty()) {
        return;
    }

    if (target.components()) {
        std::vector<command::Vertex::Change> changes;
        bool moved = false;
        for (std::size_t index = 0; index < target.parts().size() && index < shapes.size(); index++) {
            const manipulator::Target::Part& part = target.parts()[index];
            const command::Vertex::Shape after = command::Vertex::Shape::of(part);
            moved = moved || !after.same(shapes[index]);
            changes.push_back({.part = part, .before = shapes[index], .after = after});
        }
        std::vector<command::Vertex::CurveChange> curves;
        for (std::size_t index = 0; index < target.curves().size() && index < outlines.size(); index++) {
            const manipulator::Target::CurvePart& part = target.curves()[index];
            const command::Vertex::Outline after = command::Vertex::Outline::of(part);
            moved = moved || !after.same(outlines[index]);
            curves.push_back({.part = part, .before = outlines[index], .after = after});
        }
        if (moved) {
            commands_->push(boost::make_shared<command::Vertex>(changes, curves, modeName(mode_)));
        }
        return;
    }

    // every placement is recorded in full, because a turn or a scale of several items
    // moves each one's translation as well
    const scene::Items& items = target.items();
    std::vector<command::Transform::Change> changes;
    bool moved = false;
    for (std::size_t index = 0; index < items.size() && index < before.size(); index++) {
        const v3d::type::Transform after = items[index]->placement();
        moved = moved || !command::same(after, before[index]);
        changes.push_back({.item = items[index], .before = before[index], .after = after});
    }
    if (moved) {
        std::vector<command::Keyed::Change> keyed;
        for (const command::Transform::Change& change : changes) {
            scene::Pose was = scene::pose(*change.item);
            was.placement = change.before;
            keyed.push_back({ .item = change.item, .before = was, .after = scene::pose(*change.item) });
        }
        commands_->push(command::Keyed::record(scene_, boost::make_shared<command::Transform>(changes, modeName(mode_)), keyed));
        return;
    }
    // a drag that measured nothing still wrote each placement, from where the evaluation put
    // it, so each item is given back the placement it held
    for (const command::Transform::Change& change : changes) {
        change.item->placement(change.before);
    }
}

/**
 **/
void Transform::hover(const glm::vec2& position) {
    const boost::shared_ptr<manipulator::Manipulator> current = manipulator();
    if (!current) {
        return;
    }
    if (!scene_ || !view_) {
        current->active(false);
        return;
    }

    const manipulator::Target target = this->target();
    manipulator::Manipulator::Axis axis = manipulator::Manipulator::Axis::None;
    if (target.empty() || !current->grab(target, screen(), position, &axis)) {
        current->active(false);
        return;
    }
    current->axis(axis);
    current->active(true);
}

/**
 **/
void Transform::motion(const glm::vec2& position) {
    const boost::shared_ptr<manipulator::Manipulator> current = manipulator();
    // a drag moves the set whose handle was grabbed rather than whatever is selected
    // now, so that a selection changing mid gesture cannot leave the record naming one
    // set and the movement landing on another
    if (dragging_ && current && !dragged_.empty() && view_) {
        current->apply(&dragged_, screen(), last_, position);
    } else {
        hover(position);
    }
    last_ = position;
}

/**
 **/
void Transform::button(unsigned int button, bool pressed, const glm::vec2& position) {
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
    const boost::shared_ptr<manipulator::Manipulator> current = manipulator();
    if (!current || !scene_ || !view_) {
        return;
    }

    const manipulator::Target target = this->target();
    if (target.empty()) {
        return;
    }

    manipulator::Manipulator::Axis axis = manipulator::Manipulator::Axis::None;
    if (!current->grab(target, screen(), position, &axis)) {
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
    outlines_.clear();
    if (target.components()) {
        for (const manipulator::Target::Part& part : target.parts()) {
            shapes_.push_back(command::Vertex::Shape::of(part));
        }
        for (const manipulator::Target::CurvePart& part : target.curves()) {
            outlines_.push_back(command::Vertex::Outline::of(part));
        }
    } else {
        for (const boost::shared_ptr<scene::item::Item>& item : target.items()) {
            before_.push_back(item->placement());
        }
    }
}

};  // namespace v3d::editor::tool
