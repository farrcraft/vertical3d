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
 * Pushing never applies anything - a command arrives already done, per ADR-0016 - and
 * pushing abandons whatever had been undone, because a new change makes a branch of the
 * history that was never taken.
 *
 * The stack has a capacity so that a session cannot grow one without bound; the oldest
 * commands are dropped first, which is the part of the history furthest from being
 * undone.
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
     * Forget the history, which is what opening a document does. Nothing is undone on
     * the way out: the scene the commands describe is being replaced.
     **/
    void clear() noexcept;

 private:
    std::vector<boost::shared_ptr<Command>> done_;
    std::vector<boost::shared_ptr<Command>> undone_;
    std::size_t capacity_;
};

};  // namespace v3d::editor
