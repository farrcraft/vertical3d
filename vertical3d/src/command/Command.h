/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

namespace v3d::editor {

/**
 * One undoable unit of editing.
 *
 * A command is a record of a change that has already been made rather than a request to
 * make one, per ADR-0016: it is pushed onto the stack once the change is complete, and
 * the stack calls undo() and redo() on it from then on. What one unit is belongs to
 * whatever made the change - for a drag it is the whole gesture and not the motion
 * event.
 **/
class Command {
 public:
    virtual ~Command() { }

    /**
     * Put back what the scene held before the change.
     **/
    virtual void undo() = 0;

    /**
     * Make the change again. This is also how the change is made the first time, so
     * that the first do and a redo cannot drift apart.
     **/
    virtual void redo() = 0;

    /**
     * @return what to call the change, for the log and for a menu label
     **/
    virtual std::string name() const = 0;
};

};  // namespace v3d::editor
