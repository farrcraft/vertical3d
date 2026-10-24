/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "Command.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 * The editor's history: what has been done, and what has been undone out of it.
 *
 * Pushing never applies anything, because a command arrives already done. Pushing also
 * discards whatever had been undone. A new change starts a new branch of the history, so
 * the undone commands cannot be redone.
 *
 * The stack has a capacity, so a session's history cannot grow without bound. The oldest
 * commands are dropped first, because they are the last to be undone.
 *
 * The stack also remembers the position it was at when the document was last saved. The
 * document is dirty at any other position. A saved position can be lost: when the command
 * that reached it is dropped off the far end, or when a push discards the undone commands it
 * lay among. A lost position is never reached again, so the document stays dirty until the
 * next save.
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
     * scene the commands describe is being replaced. The empty stack counts as saved, because
     * the document has just been read.
     **/
    void clear() noexcept;

    /**
     * Record that the document now matches the stack's present position, as a save does.
     **/
    void markSaved() noexcept;

    /**
     * @return whether the document differs from what was last saved
     **/
    bool dirty() const noexcept;

 private:
    std::vector<boost::shared_ptr<Command>> done_;
    std::vector<boost::shared_ptr<Command>> undone_;
    std::size_t capacity_;
    // how many commands were done when the document was saved, or nothing once that
    // position cannot be reached again
    std::optional<std::size_t> saved_;
};

};  // namespace v3d::editor
