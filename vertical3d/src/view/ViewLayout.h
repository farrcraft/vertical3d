/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <vector>

#include "../../../api/asset/Json.h"
#include "../../../api/log/Logger.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

/**
 * How the window is divided into viewports, loaded from data/layout.json.
 *
 * The config describes the window as a tree of nested splits with a viewport at each
 * leaf. There are no widgets here, so the tree is flattened into one pixel region per
 * viewport, and that region is what a pass draws into.
 *
 * A split divides its area evenly between its children, and the divisions are fixed.
 **/
class ViewLayout final {
 public:
    /**
     * One viewport of the layout - which camera profile it shows and where it is.
     **/
    struct View final {
        View();

        std::string camera;  /**< the profile name, which CameraProfiles resolves **/
        glm::vec4 region;    /**< x, y, width, height in pixels, filled in by resize() **/
    };

    /**
     * @param logger
     **/
    explicit ViewLayout(const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     * Read the layout tree.
     * A layout with no viewport in it is rejected - there would be nothing to draw.
     *
     * @param config the parsed layout.json
     * @return whether the tree was understood
     **/
    bool load(const boost::shared_ptr<v3d::asset::Json>& config);

    /**
     * Divide a window of this size between the views.
     *
     * A split whose share rounds to nothing still leaves its children a region of zero
     * width or height rather than a negative one, so a window dragged to nothing does
     * not produce a viewport the recorder would reject.
     **/
    void resize(int width, int height);

    /**
     * Divide a region of the window between the views, rather than the whole of it. The
     * menu bar takes a strip off the top, and the views have the rest.
     **/
    void resize(const glm::vec4& region);

    /**
     * @return the views, in the order the document listed them
     **/
    const std::vector<View>& views() const noexcept;

    /**
     * @return the layout's name, for logs
     **/
    const std::string& name() const noexcept;

    /**
     * Which view a point in the window falls in.
     * @return an index into views(), or views().size() when it falls in none
     **/
    std::size_t viewAt(float x, float y) const noexcept;

 private:
    /**
     * One node of the tree: either a split with children, or a leaf naming a camera.
     **/
    struct Node final {
        Node();

        bool vertical;             /**< a vertical split stacks its children **/
        std::size_t view;          /**< index into views_, for a leaf **/
        std::vector<Node> children;
    };

    /**
     * Read one node and everything below it, adding a view for each leaf.
     * @return whether the subtree was understood
     **/
    bool loadNode(const boost::json::object& entry, Node* into);

    /**
     * Give a node the region it covers, and divide it among its children.
     **/
    void place(const Node& node, const glm::vec4& region);

    boost::shared_ptr<v3d::log::Logger> logger_;
    std::string name_;
    Node root_;
    std::vector<View> views_;
};

};  // namespace v3d::editor
