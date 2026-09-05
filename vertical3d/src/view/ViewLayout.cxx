/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "ViewLayout.h"

#include <cstddef>
#include <string>
#include <vector>

namespace v3d::editor {

/**
 **/
ViewLayout::View::View() :
region(0.0f, 0.0f, 0.0f, 0.0f) {
}

/**
 **/
ViewLayout::Node::Node() :
vertical(true),
view(0) {
}

/**
 **/
ViewLayout::ViewLayout(const boost::shared_ptr<v3d::log::Logger>& logger) :
    logger_(logger) {
}

/**
 **/
bool ViewLayout::load(const boost::shared_ptr<v3d::asset::Json>& config) {
    if (!config) {
        return false;
    }
    auto const doc = config->document();
    if (!doc.contains("layout") || !doc.at("layout").is_object()) {
        logger_->get()->error("Missing layout in the layout config");
        return false;
    }
    auto const layout = doc.at("layout").as_object();
    if (layout.contains("name")) {
        name_ = boost::json::value_to<std::string>(layout.at("name"));
    }
    if (!layout.contains("root") || !layout.at("root").is_object()) {
        logger_->get()->error("A layout needs a root node");
        return false;
    }

    views_.clear();
    root_ = Node();
    if (!loadNode(layout.at("root").as_object(), &root_)) {
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
    // a leaf names the camera it shows; anything else is a split
    if (entry.contains("camera")) {
        View view;
        view.camera = boost::json::value_to<std::string>(entry.at("camera"));
        into->view = views_.size();
        views_.push_back(view);
        return true;
    }

    if (!entry.contains("children") || !entry.at("children").is_array()) {
        logger_->get()->error("A layout node is either a camera or a split with children");
        return false;
    }
    // horizontal puts its children side by side; vertical stacks them
    const std::string split = entry.contains("split") ?
        boost::json::value_to<std::string>(entry.at("split")) : std::string("vertical");
    into->vertical = (split != "horizontal");

    for (auto const& child : entry.at("children").as_array()) {
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
    resize(glm::vec4(0.0f, 0.0f,
        static_cast<float>(width > 0 ? width : 0),
        static_cast<float>(height > 0 ? height : 0)));
}

/**
 **/
void ViewLayout::resize(const glm::vec4& region) {
    place(root_, glm::vec4(region.x, region.y,
        region.z > 0.0f ? region.z : 0.0f,
        region.w > 0.0f ? region.w : 0.0f));
}

/**
 **/
void ViewLayout::place(const Node& node, const glm::vec4& region) {
    if (node.children.empty()) {
        if (node.view < views_.size()) {
            views_[node.view].region = region;
        }
        return;
    }

    const std::size_t count = node.children.size();
    // the last child takes what integer division left over, so the children cover the
    // whole region rather than leaving a seam of undrawn pixels down the middle
    float offset = node.vertical ? region.y : region.x;
    const float total = node.vertical ? region.w : region.z;
    const float end = offset + total;

    for (std::size_t index = 0; index < count; index++) {
        const bool last = (index + 1 == count);
        const float extent = last ? (end - offset) :
            static_cast<float>(static_cast<int>(total / static_cast<float>(count)));
        glm::vec4 child = region;
        if (node.vertical) {
            child.y = offset;
            child.w = extent > 0.0f ? extent : 0.0f;
        } else {
            child.x = offset;
            child.z = extent > 0.0f ? extent : 0.0f;
        }
        place(node.children[index], child);
        offset += extent;
    }
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
