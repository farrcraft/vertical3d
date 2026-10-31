/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "ToolCommands.h"

#include <api/event/Event.h>
#include <api/event/State.h>
#include <vertical3d/src/app/CommandTable.h>
#include <vertical3d/src/app/Context.h>

#include <string>

#include <glm/vec2.hpp>

namespace v3d::editor {

/**
 **/
ToolCommands::ToolCommands(const Context& context) :
    context_(context) {
}

/**
 **/
void ToolCommands::registerCommands(CommandTable* table) const {
    for (const char* mask : { "object", "vertex", "edge", "face", "mesh", "light", "camera", "curve", "handle" }) {
        table->press(std::string("select::mask::") + mask, [this, mask]() { selectMask(mask); });
    }

    // the Camera Manipulators menu latches a move, so a drag with no modifier drives the
    // camera until the move is chosen again or a transform mode is
    for (const char* move : { "truck", "dolly", "zoom", "pan", "pitch" }) {
        table->press(std::string("camera::") + move, [this, move]() { context_.cameraTool->latch(move); });
    }
    for (const char* mode : { "select", "translate", "rotate", "scale" }) {
        table->press(std::string("transform::") + mode, [this, mode]() { transformMode(mode); });
    }

    // these are held: the modifier going down chooses what a drag of the primary button
    // performs, and it coming up puts the tool back to whatever is latched
    for (const char* move : { "zoom", "dolly", "truck" }) {
        table->hold(std::string("view::camera::") + move, [this, move](const v3d::event::Event& event) {
            cameraMode(move, event.state() != v3d::event::State::Released);
        });
    }
    // pan is a button of its own, so holding it is the drag. It drives the camera alone,
    // whatever the primary button is doing
    table->hold("view::camera::pan", [this](const v3d::event::Event& event) {
        const bool pressed = event.state() != v3d::event::State::Released;
        cameraMode("pan", pressed);
        context_.cameraTool->button(1, pressed, context_.pointer ? context_.pointer->cursor() : glm::vec2(0.0F, 0.0F));
    });
    // held: while it is down a click adds to the selection or takes away from it
    table->hold("select::add", [this](const v3d::event::Event& event) {
        context_.selectTool->adding(event.state() != v3d::event::State::Released);
    });
}

/**
 **/
void ToolCommands::transformMode(const std::string& name) const {
    // choosing a manipulator asks for the handles, so a latched camera move lets them go
    context_.cameraTool->unlatch();
    context_.transformTool->activate(name);
}

/**
 **/
void ToolCommands::selectMask(const std::string& name) const {
    context_.selectTool->activate(name);
    context_.transformTool->mask(context_.selectTool->mask());
}

/**
 **/
void ToolCommands::cameraMode(const std::string& name, bool pressed) const {
    if (pressed) {
        context_.cameraTool->activate(name);
    } else {
        context_.cameraTool->deactivate(name);
    }
}

};  // namespace v3d::editor
