/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/kind/Json.h>
#include <api/log/Logger.h>

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

/**
 * How the window is divided into viewports, loaded from data/layout.json.
 *
 * The config describes the window as a tree of nested splits with a viewport at each
 * leaf. The tree is flattened into one pixel region per viewport, and a pass draws into
 * that region.
 *
 * A split divides its area between its children by their shares. A share is a weight against
 * the other children of the same split, and a child with none has a share of one, so a split
 * whose children name none divides evenly. A split with no children is refused, at any depth.
 *
 * The divider between two children of a split can be dragged. It moves weight from one of the
 * two to the other, so their sum, and every other child, stays as it was. The shares live as
 * long as the layout, and are not written back to the config.
 **/
class ViewLayout final {
 public:
    /**
     * One viewport of the layout: which camera profile it shows, and where it is.
     **/
    struct View final {
        View();

        std::string camera;  /**< the profile name, which CameraProfiles resolves **/
        /**
         * What the view is called, which is also its pass name, so no two views share one.
         * It is the profile name, with " 2", " 3" and so on after the profile's later views.
         **/
        std::string name;
        glm::vec4 region;    /**< x, y, width, height in pixels, filled in by resize() **/
    };

    /**
     * @param logger
     **/
    explicit ViewLayout(const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     * Read the layout tree.
     * A layout with no viewport in it is rejected, because it would draw nothing.
     *
     * @param config the parsed layout.json
     * @return whether the tree was understood
     **/
    bool load(const boost::shared_ptr<v3d::asset::kind::Json>& config);

    /**
     * Divide a window of this size between the views.
     *
     * A split whose share rounds to nothing still leaves its children a region of zero
     * width or height rather than a negative one, so a window dragged to nothing does
     * not produce a viewport the recorder would reject.
     **/
    void resize(int width, int height);

    /**
     * Divide a region of the window between the views, not the whole of it. The menu bar
     * and the toolbars cover strips along the top and left edges, and the views have the
     * rest.
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

    /** How near a divider, in pixels, a press has to be to take hold of it. **/
    static constexpr float dividerReach = 3.0F;

    /** The least part of its split's extent a child keeps when a divider is dragged. **/
    static constexpr float minimumShare = 0.05F;

    /**
     * Take hold of the divider at a point of the window, if one is within dividerReach of it.
     * Where the dividers of two splits meet, the outer split's is taken.
     *
     * @return whether a divider was taken
     **/
    bool grab(float x, float y);

    /**
     * @return whether a divider is held
     **/
    bool grabbed() const noexcept;

    /**
     * Move the held divider to a point of the window, and place the views again in the region
     * last given to resize(). The divider moves along its split only, and each of the two
     * children beside it keeps at least minimumShare of the split's extent.
     **/
    void drag(float x, float y);

    /**
     * Let go of the held divider.
     **/
    void release() noexcept;

 private:
    /**
     * One node of the tree: either a split with children, or a leaf naming a camera.
     **/
    struct Node final {
        Node();

        bool vertical;             /**< a vertical split stacks its children **/
        std::size_t view;          /**< index into views_, for a leaf **/
        float share;               /**< the node's weight among its split's children **/
        glm::vec4 region;          /**< where the node was last placed **/
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
    void place(Node* node, const glm::vec4& region);

    /**
     * Take hold of the divider at a point in a node or below it, outer splits first.
     * @return whether one was taken
     **/
    bool grabIn(Node* node, float x, float y);

    boost::shared_ptr<v3d::log::Logger> logger_;
    std::string name_;
    Node root_;
    std::vector<View> views_;
    glm::vec4 area_;
    // the split whose divider is held, and the child before the divider. A pointer into root_,
    // which only load() changes, and load() lets go first
    Node* held_;
    std::size_t heldAfter_;
};

};  // namespace v3d::editor
