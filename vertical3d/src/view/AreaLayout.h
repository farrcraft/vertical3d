/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <boost/json.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

/**
 * How the window is divided into areas, each of which holds editors.
 *
 * The layout is a tree of splits whose leaves are areas. A split puts its children side by
 * side or stacks them, and divides its room between them by their shares. A share is a weight
 * against the other children of the same split, and a child with none has a share of one.
 *
 * An area holds one or more editors and shows one of them, its chosen editor. An editor is held
 * as its kind and the state it saves, so a viewport and an outliner are held the same way. Every
 * area and every editor has an id that lasts while the layout does, so a caller can follow one
 * through a change that renumbers the areas.
 *
 * Areas are separated by a gap, and the same gap runs around the edge of the region the layout
 * is given. The gap between two children of a split is a divider, which can be dragged. Moving a
 * divider trades weight between the two children beside it, so every other child keeps its
 * size, and neither of the two is made smaller than the least size of what it holds.
 *
 * A change to the tree, such as a split or a join, renumbers the areas, lets go of a held
 * divider and restores a maximised area. The shares, the editors and the tree are what save()
 * writes. The regions, the held divider and the maximised area are not.
 **/
class AreaLayout final {
 public:
    /**
     * The name config.json gives the kind of a layout document.
     **/
    static constexpr std::string_view DOCUMENT = "layout";

    /**
     * One editor in an area.
     **/
    struct Editor final {
        std::size_t id = 0;          /**< unique among the layout's editors while it lasts **/
        std::string kind;            /**< what the editor is, such as "viewport" **/
        boost::json::object state;   /**< what the editor saves of itself, its kind included **/
    };

    /**
     * One leaf of the tree.
     **/
    struct Area final {
        std::size_t id = 0;            /**< unique among the layout's areas while it lasts **/
        std::vector<Editor> editors;   /**< never empty **/
        std::size_t chosen = 0;        /**< the editor the area shows **/
        glm::vec4 region{ 0.0F };      /**< x, y, width, height in pixels; empty while another area is maximised **/

        /**
         * @return the editor the area shows
         **/
        const Editor& editor() const;
    };

    /**
     * The sizes the layout keeps to, in pixels.
     **/
    struct Metrics final {
        static constexpr float GAP = 3.0F;
        static constexpr float REACH = 4.0F;
        /** An area's header and 40 pixels beneath it. **/
        static constexpr float LEAST = 68.0F;

        float gap = GAP;       /**< between two areas, and around the edge **/
        float reach = REACH;   /**< how far either side of a gap a press takes the divider **/
        float least = LEAST;   /**< the least width or height an area is dragged to **/
    };

    /**
     * A side of an area.
     **/
    enum class Edge : std::uint8_t { Left, Right, Top, Bottom };

    /**
     * The gap between two children of a split.
     **/
    struct Divider final {
        glm::vec4 region{ 0.0F };   /**< the gap, as x, y, width, height **/
        bool across = false;        /**< whether it runs from left to right, between stacked children **/
    };

    /**
     * @param logger
     **/
    explicit AreaLayout(const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     * Read a layout: an object with a root node and an optional name.
     *
     * A node is a split, with `split` and `children`, or an area, with `editors` and an
     * optional `chosen`. Either may have a `share`. An editor is an object with a `kind`, and the
     * whole object is kept as its state. A layout that cannot be read leaves the one held as it
     * was.
     *
     * @return whether the layout was understood
     **/
    bool load(const boost::json::object& layout);

    /**
     * @return the layout as load() reads it
     **/
    boost::json::object save() const;

    /**
     * @return the layout's name, for logs and for a list of workspaces
     **/
    const std::string& name() const noexcept;

    /**
     * Set the sizes the layout keeps to, and place the areas again.
     **/
    void metrics(const Metrics& metrics);

    /**
     * @return the sizes the layout keeps to
     **/
    const Metrics& metrics() const noexcept;

    /**
     * Divide a region of the window between the areas.
     *
     * Each extent is rounded down to a whole pixel, and the last child of a split takes what
     * that left over, so the areas and their gaps cover the region exactly. A region too small
     * for every area gives some of them an empty one rather than a negative one.
     *
     * @param region x, y, width and height, in pixels
     **/
    void resize(const glm::vec4& region);

    /**
     * @return the areas, in the order a depth first walk of the tree meets them
     **/
    const std::vector<Area>& areas() const noexcept;

    /**
     * @return the index in areas() of the area with an id, or areas().size() when there is none
     **/
    std::size_t find(std::size_t id) const noexcept;

    /**
     * Which area a point of the window falls in. A point in a gap falls in none.
     * @return an index into areas(), or areas().size() when it falls in none
     **/
    std::size_t areaAt(float x, float y) const noexcept;

    /**
     * The divider a press at a point would take: one within the metrics' reach of its gap, along
     * the length of its split. Where the dividers of two splits meet, the outer split's is found.
     * No divider is found while an area is maximised.
     **/
    std::optional<Divider> divider(float x, float y) const;

    /**
     * Take hold of the divider at a point, as divider() finds it.
     * @return whether a divider was taken
     **/
    bool grab(float x, float y);

    /**
     * @return the held divider, or nothing when none is held
     **/
    std::optional<Divider> held() const;

    /**
     * @return whether a divider is held
     **/
    bool grabbed() const noexcept;

    /**
     * Move the held divider so that its gap is centred on a point, along its split only.
     *
     * Each child beside it keeps at least its least size: the metrics' least for an area, and
     * for a split what its own children need. A divider with no room to move stays where it is.
     *
     * @param snap whether to put the divider on the nearest tenth of its split
     **/
    void drag(float x, float y, bool snap = false);

    /**
     * Let go of the held divider where it is.
     **/
    void release() noexcept;

    /**
     * Put the held divider back where it was taken, and let go of it.
     **/
    void cancel();

    /**
     * Divide the room of the two children beside the divider at a point evenly between them.
     * @return whether there was a divider there
     **/
    bool even(float x, float y);

    /**
     * Give one area the whole region, or give it back to every area if that area already has
     * it. The others keep their place in the tree and have an empty region meanwhile.
     *
     * @param area an index into areas()
     **/
    void maximise(std::size_t area);

    /**
     * @return the index of the maximised area, or nothing when every area has its place
     **/
    std::optional<std::size_t> maximised() const noexcept;

    /**
     * Divide an area in two. The new area takes the half on one edge, and starts with a copy of
     * the editor the area shows. An area too small for two areas of the least size and a gap
     * is not divided.
     *
     * @param area an index into areas()
     * @param edge where the new area goes
     * @return the index of the new area, or areas().size() when none was made
     **/
    std::size_t split(std::size_t area, Edge edge);

    /**
     * Give one area's room to a neighbour, and close the editors it held. The two have to be
     * children of the same split, side by side.
     *
     * @param keep the area that takes the room, an index into areas()
     * @param gone the area that goes, an index into areas()
     * @return whether the two were joined
     **/
    bool join(std::size_t keep, std::size_t gone);

    /**
     * Exchange what two areas hold, leaving each area where it is.
     * @return whether both indices were areas
     **/
    bool swap(std::size_t first, std::size_t second);

    /**
     * Add an editor to an area and show it.
     *
     * @param state what the editor saves of itself, which names its kind
     * @return the new editor's id, or nothing when the area is not one or the state names no kind
     **/
    std::optional<std::size_t> add(std::size_t area, const boost::json::object& state);

    /**
     * Show one of an area's editors.
     * @return whether the area holds that editor
     **/
    bool choose(std::size_t area, std::size_t editor);

    /**
     * Close one of an area's editors. An area's only editor is not closed, since an area always
     * shows one. Closing the editor shown shows the one before it.
     *
     * @return whether the editor was closed
     **/
    bool close(std::size_t area, std::size_t editor);

    /**
     * Move one of an area's editors to another area, where it is shown. An area left with no
     * editor gives its room to a neighbour.
     *
     * @return whether the editor was moved
     **/
    bool move(std::size_t from, std::size_t editor, std::size_t to);

    /**
     * Move one of an area's editors into a new area on one edge of another, as when a tab is
     * dropped on that edge. An area left with no editor gives its room to a neighbour. An area's
     * only editor dropped on its own edge stays where it is.
     *
     * @return whether the editor was moved
     **/
    bool drop(std::size_t from, std::size_t editor, std::size_t to, Edge edge);

 private:
    /**
     * One node of the tree: a split with children, or an area.
     **/
    struct Node final {
        bool vertical = true;            /**< a vertical split stacks its children **/
        float share = 1.0F;              /**< the node's weight among its split's children **/
        std::size_t area = 0;            /**< an index into areas_, for a leaf **/
        glm::vec4 region{ 0.0F };        /**< where the node was last placed **/
        std::vector<Node> children;      /**< empty for an area **/
    };

    /**
     * Where a node sits: its split, and its place among that split's children. The root has no
     * split.
     **/
    struct Spot final {
        Node* split = nullptr;
        std::size_t at = 0;
        Node* node = nullptr;
    };

    /**
     * Read one node and everything below it, adding an area for each leaf.
     * @return whether the subtree was understood
     **/
    bool loadNode(const boost::json::object& entry, Node* into, std::vector<Area>* areas);

    /**
     * Read the editors of an area and the one it shows.
     * @return whether the area was understood
     **/
    bool loadArea(const boost::json::object& entry, Area* into);

    /**
     * @return a node and everything below it, as load() reads it
     **/
    boost::json::object saveNode(const Node& node) const;

    /**
     * Give a node the region it covers, and divide it among its children.
     **/
    void place(Node* node, const glm::vec4& region);

    /**
     * Place every area again in the region last given to resize().
     **/
    void place();

    /**
     * @return the least extent a node takes along one axis
     **/
    float least(const Node& node, bool vertical) const;

    /**
     * A divider found at a point: the path of child indices from the root to its split, and the
     * child before it.
     **/
    struct Found final {
        std::vector<std::size_t> path;
        std::size_t after = 0;
    };

    /**
     * Find the divider at a point in a node or below it, outer splits first.
     * @param path the path to the node, which the search extends and restores
     **/
    std::optional<Found> dividerIn(const Node& node, float x, float y, std::vector<std::size_t>* path) const;

    /**
     * Find the divider at a point, unless an area is maximised.
     **/
    std::optional<Found> dividerAt(float x, float y) const;

    /**
     * @return the node at the end of a path of child indices from the root
     **/
    Node* walk(const std::vector<std::size_t>& path);

    /**
     * @return the gap after one child of a split
     **/
    static Divider gapAfter(const Node& split, std::size_t index);

    /**
     * @return where the leaf of an area sits, or a spot with no node
     **/
    Spot spot(std::size_t area);

    /**
     * Put a new area holding one editor on an edge of an area.
     * @return the new area's id, or nothing when it was too small to divide
     **/
    std::optional<std::size_t> divide(std::size_t area, Edge edge, const Editor& editor);

    /**
     * Take an area's leaf out of the tree, giving its share to a sibling beside it.
     * @param heir the sibling's place in the split, or nothing for the one before it, or the
     *        one after it when it is first
     **/
    void remove(std::size_t area, std::optional<std::size_t> heir = std::nullopt);

    /**
     * Flatten what a change to the tree left: a split with one child becomes that child, and a
     * split inside another that runs the same way gives its children to that one.
     **/
    static void tidy(Node* node);

    /**
     * Renumber the areas in the order the tree meets them, let go of a held divider, restore a
     * maximised area, and place the areas again.
     **/
    void rebuild();

    /**
     * Gather the areas below a node into a list, in the tree's order.
     **/
    void collect(Node* node, std::vector<Area>* into);

    /**
     * Let go of a held divider and give every area its place back, before a change to the tree.
     **/
    void restore();

    boost::shared_ptr<v3d::log::Logger> logger_;
    std::string name_;
    Node root_;
    std::vector<Area> areas_;
    Metrics metrics_;
    glm::vec4 region_{ 0.0F };
    std::size_t nextArea_ = 1;
    std::size_t nextEditor_ = 1;
    std::optional<std::size_t> maximised_;   // an area id
    // the split whose divider is held, and the child before it. A pointer into root_, which
    // only a change to the tree moves, and every change lets go first
    Node* held_ = nullptr;
    std::size_t heldAfter_ = 0;
    std::vector<float> heldShares_;   // the split's shares when it was taken, for cancel()
};

};  // namespace v3d::editor
