/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <vector>

#include "Command.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 * The editor's history: what has been done, and what has been undone out of it.
 *
 * Pushing never applies anything, because a command arrives already done. Pushing
 * discards whatever had been undone: a new change starts a new branch of the history, so
 * the undone commands cannot be redone.
 *
 * The stack has a capacity so that a session's history cannot grow without bound. The
 * oldest commands are dropped first, since they are furthest from being undone.
 **/
class CommandStack final {
 public:
    /**
     * @param capacity how many done commands to keep, at least one
     **/
    explicit CommandStack(std::size_t capacity = 64);

    /**
     * Record a change that has already been made.
     **/
    void push(const boost::shared_ptr<Command>& command);

    /**
     * Undo the most recent command.
     * @return the command undone, or an empty pointer if there was none
     **/
    boost::shared_ptr<Command> undo();

    /**
     * Redo the most recently undone command.
     * @return the command redone, or an empty pointer if there was none
     **/
    boost::shared_ptr<Command> redo();

    /**
     **/
    bool canUndo() const noexcept;

    /**
     **/
    bool canRedo() const noexcept;

    /**
     **/
    std::size_t undoDepth() const noexcept;

    /**
     **/
    std::size_t redoDepth() const noexcept;

    /**
     * Clear the history, as opening a document does. Nothing is undone first, because the
     * scene the commands describe is being replaced.
     **/
    void clear() noexcept;

 private:
    std::vector<boost::shared_ptr<Command>> done_;
    std::vector<boost::shared_ptr<Command>> undone_;
    std::size_t capacity_;
};

};  // namespace v3d::editor
