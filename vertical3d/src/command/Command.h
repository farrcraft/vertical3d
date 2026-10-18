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
 * A command is pushed onto the stack once its change has been made, and the stack calls
 * undo() and redo() on it from then on. Pushing never applies it. Whatever made the change
 * decides how large one unit is: for a drag it is the whole gesture, not one motion event.
 **/
class Command {
 public:
    virtual ~Command() { }

    /**
     * Put back what the scene held before the change.
     **/
    virtual void undo() = 0;

    /**
     * Make the change again. A command whose change is not already made calls this to make
     * it the first time, so that the first do and a redo run the same code. A
     * TransformCommand is the exception: its drag moved the mesh before it was made.
     **/
    virtual void redo() = 0;

    /**
     * @return what to call the change, for the log and for a menu label
     **/
    virtual std::string name() const = 0;
};

};  // namespace v3d::editor
