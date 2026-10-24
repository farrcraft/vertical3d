/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "ViewLayout.h"

#include <api/asset/Json.h>
#include <api/asset/kind/Json.h>
#include <api/log/Logger.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace v3d::editor {

/**
 **/
ViewLayout::View::View() :
region(0.0F, 0.0F, 0.0F, 0.0F) {
}

/**
 **/
ViewLayout::Node::Node() :
vertical(true),
view(0),
share(1.0F),
region(0.0F, 0.0F, 0.0F, 0.0F) {
}

/**
 **/
ViewLayout::ViewLayout(const boost::shared_ptr<v3d::log::Logger>& logger) :
    logger_(logger),
    area_(0.0F, 0.0F, 0.0F, 0.0F),
    held_(nullptr),
    heldAfter_(0) {
}

/**
 **/
bool ViewLayout::load(const boost::shared_ptr<v3d::asset::kind::Json>& config) {
    if (!config) {
        return false;
    }
    auto const doc = config->document();
    const boost::json::object* found = v3d::asset::readObject(doc, "layout");
    if (found == nullptr) {
        logger_->get()->error("Missing layout in the layout config");
        return false;
    }
    auto const layout = *found;
    if (layout.contains("name")) {
        const std::optional<std::string> name = v3d::asset::readString(layout, "name");
        if (!name) {
            logger_->get()->error("A layout's name is a string");
            return false;
        }
        name_ = *name;
    }
    const boost::json::object* root = v3d::asset::readObject(layout, "root");
    if (root == nullptr) {
        logger_->get()->error("A layout needs a root node");
        return false;
    }

    release();
    views_.clear();
    root_ = Node();
    if (!loadNode(*root, &root_)) {
        return false;
    }
    if (views_.empty()) {
        logger_->get()->error("A layout with no viewport in it draws nothing");
        return false;
    }
    return true;
}

/**
 **/
bool ViewLayout::loadNode(const boost::json::object& entry, Node* into) {
    if (entry.contains("share")) {
        const std::optional<double> share = v3d::asset::readNumber(entry, "share");
        if (!share || !std::isfinite(*share) || *share <= 0.0) {
            logger_->get()->error("A layout node's share is a number above zero");
            return false;
        }
        into->share = static_cast<float>(*share);
    }

    // a leaf names the camera it shows; anything else is a split
    if (entry.contains("camera")) {
        const std::optional<std::string> camera = v3d::asset::readString(entry, "camera");
        if (!camera) {
            logger_->get()->error("A layout node's camera is a name");
            return false;
        }
        View view;
        view.camera = *camera;
        // a layout may show one profile twice, and each view draws in a pass of its own
        const std::size_t shown = static_cast<std::size_t>(std::ranges::count_if(views_,
            [&camera](const View& earlier) { return earlier.camera == *camera; }));
        view.name = shown == 0 ? *camera : *camera + " " + std::to_string(shown + 1);
        into->view = views_.size();
        views_.push_back(view);
        return true;
    }

    const boost::json::array* children = v3d::asset::readArray(entry, "children");
    if (children == nullptr) {
        logger_->get()->error("A layout node is either a camera or a split with children");
        return false;
    }
    // place() treats a node with no children as a leaf, so an empty split would take a view
    if (children->empty()) {
        logger_->get()->error("A layout split has no children");
        return false;
    }
    // horizontal puts its children side by side; vertical stacks them
    const std::optional<std::string> split = v3d::asset::readString(entry, "split");
    if (entry.contains("split") && !split) {
        logger_->get()->error("A layout node's split is horizontal or vertical");
        return false;
    }
    into->vertical = (split.value_or("vertical") != "horizontal");

    for (auto const& child : *children) {
        if (!child.is_object()) {
            logger_->get()->error("Unrecognized layout node");
            return false;
        }
        Node node;
        if (!loadNode(child.as_object(), &node)) {
            return false;
        }
        into->children.push_back(node);
    }
    return true;
}

/**
 **/
void ViewLayout::resize(int width, int height) {
    resize(glm::vec4(0.0F, 0.0F,
        static_cast<float>(width > 0 ? width : 0),
        static_cast<float>(height > 0 ? height : 0)));
}

/**
 **/
void ViewLayout::resize(const glm::vec4& region) {
    area_ = glm::vec4(region.x, region.y,
        region.z > 0.0F ? region.z : 0.0F,
        region.w > 0.0F ? region.w : 0.0F);
    place(&root_, area_);
}

/**
 **/
void ViewLayout::place(Node* node, const glm::vec4& region) {
    node->region = region;
    if (node->children.empty()) {
        if (node->view < views_.size()) {
            views_[node->view].region = region;
        }
        return;
    }

    const std::size_t count = node->children.size();
    float shares = 0.0F;
    for (const Node& child : node->children) {
        shares += child.share;
    }
    // the last child takes what rounding each share down to a whole pixel left over, so the
    // children cover the whole region rather than leaving a gap of undrawn pixels
    float offset = node->vertical ? region.y : region.x;
    const float total = node->vertical ? region.w : region.z;
    const float end = offset + total;

    for (std::size_t index = 0; index < count; index++) {
        const bool last = (index + 1 == count);
        const float extent = last ? (end - offset) :
            std::floor(total * node->children[index].share / shares);
        glm::vec4 child = region;
        if (node->vertical) {
            child.y = offset;
            child.w = extent > 0.0F ? extent : 0.0F;
        } else {
            child.x = offset;
            child.z = extent > 0.0F ? extent : 0.0F;
        }
        place(&node->children[index], child);
        offset += extent;
    }
}

/**
 **/
bool ViewLayout::grab(float x, float y) {
    release();
    return grabIn(&root_, x, y);
}

/**
 **/
bool ViewLayout::grabIn(Node* node, float x, float y) {
    const glm::vec4& region = node->region;
    if (x < region.x - dividerReach || x > region.x + region.z + dividerReach ||
        y < region.y - dividerReach || y > region.y + region.w + dividerReach) {
        return false;
    }
    // a divider is where one child ends and the next begins, along the split's axis
    const float along = node->vertical ? y : x;
    for (std::size_t index = 0; index + 1 < node->children.size(); index++) {
        const glm::vec4& before = node->children[index].region;
        const float divider = node->vertical ? before.y + before.w : before.x + before.z;
        if (std::abs(along - divider) <= dividerReach) {
            held_ = node;
            heldAfter_ = index;
            return true;
        }
    }
    return std::ranges::any_of(node->children, [this, x, y](Node& child) { return grabIn(&child, x, y); });
}

/**
 **/
bool ViewLayout::grabbed() const noexcept {
    return held_ != nullptr;
}

/**
 **/
void ViewLayout::drag(float x, float y) {
    if (held_ == nullptr) {
        return;
    }
    Node& first = held_->children[heldAfter_];
    Node& second = held_->children[heldAfter_ + 1];
    const bool vertical = held_->vertical;
    const float start = vertical ? first.region.y : first.region.x;
    const float end = vertical ? second.region.y + second.region.w : second.region.x + second.region.z;
    const float floor = minimumShare * (vertical ? held_->region.w : held_->region.z);
    if (end - start <= 2.0F * floor) {
        return;
    }

    // the two keep the weight they had between them, divided where the divider now is
    const float divider = std::clamp(vertical ? y : x, start + floor, end - floor);
    const float weight = first.share + second.share;
    first.share = weight * (divider - start) / (end - start);
    second.share = weight - first.share;
    place(&root_, area_);
}

/**
 **/
void ViewLayout::release() noexcept {
    held_ = nullptr;
    heldAfter_ = 0;
}

/**
 **/
const std::vector<ViewLayout::View>& ViewLayout::views() const noexcept {
    return views_;
}

/**
 **/
const std::string& ViewLayout::name() const noexcept {
    return name_;
}

/**
 **/
std::size_t ViewLayout::viewAt(float x, float y) const noexcept {
    for (std::size_t index = 0; index < views_.size(); index++) {
        const glm::vec4& region = views_[index].region;
        if (x >= region.x && x < region.x + region.z &&
            y >= region.y && y < region.y + region.w) {
            return index;
        }
    }
    return views_.size();
}

};  // namespace v3d::editor
