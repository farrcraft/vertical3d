/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Pass.h"

#include <algorithm>
#include <string>
#include <vector>

namespace v3d::render::realtime {

    /**
     **/
    Pass::Pass(const std::string& name) :
        name_(name),
        clearColour_(0.0f, 0.0f, 0.0f, 1.0f),
        viewport_(0.0f, 0.0f, 0.0f, 0.0f),
        view_(1.0f),
        projection_(1.0f),
        clears_(true),
        depth_(false),
        sorts_(false) {
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
    void Pass::camera(const glm::mat4& view, const glm::mat4& projection) noexcept {
        view_ = view;
        projection_ = projection;
    }

    /**
     **/
    const glm::mat4& Pass::view() const noexcept {
        return view_;
    }

    /**
     **/
    const glm::mat4& Pass::projection() const noexcept {
        return projection_;
    }

    /**
     **/
    void Pass::sort(bool enabled) noexcept {
        sorts_ = enabled;
    }

    /**
     **/
    bool Pass::sorts() const noexcept {
        return sorts_;
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
    void Pass::ordered(std::vector<const DrawItem*>* into) const {
        if (into == nullptr) {
            return;
        }
        into->clear();
        into->reserve(items_.size());
        for (const DrawItem& item : items_) {
            into->push_back(&item);
        }
        if (!sorts_) {
            return;
        }
        // stable, so that items whose keys are equal keep the order they arrived in - which
        // is what a run of quads sharing a pipeline and a material depends on
        std::stable_sort(into->begin(), into->end(), [](const DrawItem* left, const DrawItem* right) {
            return left->key < right->key;
        });
    }

    /**
     **/
    void Pass::reset() noexcept {
        // the capacity is worth keeping - the next frame submits about as much as this one did
        items_.clear();
    }

};  // namespace v3d::render::realtime
