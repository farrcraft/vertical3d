/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Pass.h"

#include <string>
#include <vector>

namespace v3d::render::realtime {

    /**
     **/
    Pass::Pass(const std::string& name) :
        name_(name),
        clearColour_(0.0f, 0.0f, 0.0f, 1.0f),
        viewport_(0.0f, 0.0f, 0.0f, 0.0f),
        clears_(true),
        depth_(false) {
    }

    /**
     **/
    const std::string& Pass::name() const noexcept {
        return name_;
    }

    /**
     **/
    void Pass::clearColour(const glm::vec4& colour) noexcept {
        clearColour_ = colour;
        clears_ = true;
    }

    /**
     **/
    void Pass::keepColour() noexcept {
        clears_ = false;
    }

    /**
     **/
    bool Pass::clears() const noexcept {
        return clears_;
    }

    /**
     **/
    const glm::vec4& Pass::clearColour() const noexcept {
        return clearColour_;
    }

    /**
     **/
    void Pass::depth(bool enabled) noexcept {
        depth_ = enabled;
    }

    /**
     **/
    bool Pass::depth() const noexcept {
        return depth_;
    }

    /**
     **/
    void Pass::viewport(const glm::vec4& region) noexcept {
        viewport_ = region;
    }

    /**
     **/
    const glm::vec4& Pass::viewport() const noexcept {
        return viewport_;
    }

    /**
     **/
    void Pass::submit(const DrawItem& item) {
        items_.push_back(item);
    }

    /**
     **/
    const std::vector<DrawItem>& Pass::items() const noexcept {
        return items_;
    }

    /**
     **/
    void Pass::reset() noexcept {
        // the capacity is worth keeping - the next frame submits about as much as this one did
        items_.clear();
    }

};  // namespace v3d::render::realtime
