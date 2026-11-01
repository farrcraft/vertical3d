/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Stack.h"

#include "Command.h"

#include <algorithm>
#include <cstddef>

#include <boost/shared_ptr.hpp>

namespace v3d::editor::command {

/**
 **/
Stack::Stack(std::size_t capacity) :
    capacity_(std::max<std::size_t>(capacity, 1)),
    saved_(0) {
}

/**
 **/
void Stack::push(const boost::shared_ptr<Command>& command) {
    if (!command) {
        return;
    }
    // a saved position among the undone commands is discarded with them
    if (saved_ && *saved_ > done_.size()) {
        saved_.reset();
    }
    undone_.clear();
    done_.push_back(command);
    if (done_.size() > capacity_) {
        const std::size_t dropped = done_.size() - capacity_;
        done_.erase(done_.begin(), done_.begin() + static_cast<std::ptrdiff_t>(dropped));
        // positions count from the oldest command kept, so they shift with the drop
        if (saved_ && *saved_ < dropped) {
            saved_.reset();
        } else if (saved_) {
            *saved_ -= dropped;
        }
    }
}

/**
 **/
boost::shared_ptr<Command> Stack::undo() {
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
boost::shared_ptr<Command> Stack::redo() {
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
bool Stack::canUndo() const noexcept {
    return !done_.empty();
}

/**
 **/
bool Stack::canRedo() const noexcept {
    return !undone_.empty();
}

/**
 **/
std::size_t Stack::undoDepth() const noexcept {
    return done_.size();
}

/**
 **/
std::size_t Stack::redoDepth() const noexcept {
    return undone_.size();
}

/**
 **/
void Stack::clear() noexcept {
    done_.clear();
    undone_.clear();
    saved_ = 0;
}

/**
 **/
void Stack::markSaved() noexcept {
    saved_ = done_.size();
}

/**
 **/
void Stack::markUnsaved() noexcept {
    saved_.reset();
}

/**
 **/
bool Stack::dirty() const noexcept {
    return !saved_ || *saved_ != done_.size();
}

};  // namespace v3d::editor::command
