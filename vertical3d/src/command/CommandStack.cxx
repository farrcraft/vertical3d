/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "CommandStack.h"

#include <algorithm>
#include <cstddef>

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 **/
CommandStack::CommandStack(std::size_t capacity) :
    capacity_(std::max<std::size_t>(capacity, 1)) {
}

/**
 **/
void CommandStack::push(const boost::shared_ptr<Command>& command) {
    if (!command) {
        return;
    }
    undone_.clear();
    done_.push_back(command);
    if (done_.size() > capacity_) {
        done_.erase(done_.begin(), done_.begin() + (done_.size() - capacity_));
    }
}

/**
 **/
boost::shared_ptr<Command> CommandStack::undo() {
    if (done_.empty()) {
        return boost::shared_ptr<Command>();
    }
    boost::shared_ptr<Command> command = done_.back();
    done_.pop_back();
    command->undo();
    undone_.push_back(command);
    return command;
}

/**
 **/
boost::shared_ptr<Command> CommandStack::redo() {
    if (undone_.empty()) {
        return boost::shared_ptr<Command>();
    }
    boost::shared_ptr<Command> command = undone_.back();
    undone_.pop_back();
    command->redo();
    done_.push_back(command);
    return command;
}

/**
 **/
bool CommandStack::canUndo() const noexcept {
    return !done_.empty();
}

/**
 **/
bool CommandStack::canRedo() const noexcept {
    return !undone_.empty();
}

/**
 **/
std::size_t CommandStack::undoDepth() const noexcept {
    return done_.size();
}

/**
 **/
std::size_t CommandStack::redoDepth() const noexcept {
    return undone_.size();
}

/**
 **/
void CommandStack::clear() noexcept {
    done_.clear();
    undone_.clear();
}

};  // namespace v3d::editor
