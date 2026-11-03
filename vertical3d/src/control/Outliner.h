/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/Image.h>
#include <api/ui/Immediate.h>
#include <vertical3d/src/scene/item/Item.h>
#include <vertical3d/src/scene/Scene.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor::control {

/**
 * The outliner: an editor that lists the scene's items as the tree their parents make, under a
 * filter, and selects the item a row is clicked on.
 *
 * Every item sits under a Scene row at the root, which folds them all and selects nothing. A row
 * shows an icon for its item's kind, tinted the kind's colour, and the item's name, or its kind
 * when it has none. Siblings come in the scene's order.
 * An item with children can be folded, which hides everything beneath it. The filter keeps the
 * items whose shown name holds every word of it, ignoring case, along with their ancestors so
 * that each is shown where it sits, and opens every row it keeps. Each outliner keeps its own
 * filter and folds while the editor runs, and neither is saved.
 *
 * A click selects the item and nothing else. While the select tool adds to the selection, a
 * click selects an item that is not selected, deselects one that is, and leaves the rest.
 **/
class Outliner final {
 public:
    /**
     * The kind of an outliner editor.
     **/
    static constexpr std::string_view KIND = "outliner";

    /**
     * One row the outliner shows.
     **/
    struct Row final {
        boost::shared_ptr<scene::item::Item> item;
        unsigned int depth = 0;
        bool parent = false;   /**< whether it has children, so it can be folded **/
        bool open = false;     /**< whether its children are shown **/
    };

    /**
     * @return what a row calls an item: its name, or its kind when it has none
     **/
    static std::string label(const scene::item::Item& item);

    /**
     * What a row shows before its label: an icon's source, and the colour it is tinted.
     **/
    struct Mark final {
        std::string icon;
        glm::vec4 colour{ 1.0F };
    };

    /**
     * @return the mark of a kind of item: a camera in grey, a light in yellow, a mesh in orange,
     *         a curve in green and a group in a darker grey
     **/
    static Mark mark(scene::item::Item::Kind kind);

    /**
     * @return the mark of the Scene row
     **/
    static Mark sceneMark();

    /**
     * What gives the image an icon's source resolves to.
     **/
    using Icons = std::function<v3d::ui::Image(const std::string& source)>;

    /**
     * Resolve the rows' icons through this. With none, rows show no icon.
     **/
    void icons(const Icons& resolve);

    /**
     * The rows of a scene, in the order they are shown.
     *
     * @param filter the words a shown item's label holds, or empty for every item
     * @param folded the document ids of the items whose children are hidden. The filter opens
     *        every row it shows
     **/
    static std::vector<Row> rows(const scene::Scene& scene, const std::string& filter, const std::set<std::int64_t>& folded);

    /**
     * Draw one outliner editor into a region of the window, and select what a click asks for.
     *
     * @param editor the editor's id, which keeps its filter and folds apart from another's
     * @param adding whether a click adds to the selection
     **/
    void draw(v3d::ui::Immediate* ui, scene::Scene* scene, std::size_t editor, const glm::vec4& region, bool adding);

    /**
     * Forget the filter and folds of every editor not listed, because it has gone.
     **/
    void keep(const std::vector<std::size_t>& editors);

 private:
    /**
     * What one outliner editor keeps between frames.
     **/
    struct State final {
        std::string filter;
        std::set<std::int64_t> folded;
        bool sceneFolded = false;
    };

    /**
     * The colours each kind's icon is tinted.
     **/
    static constexpr glm::vec4 MESH{ 0.894F, 0.565F, 0.290F, 1.0F };
    static constexpr glm::vec4 CAMERA{ 0.788F, 0.800F, 0.824F, 1.0F };
    static constexpr glm::vec4 LIGHT{ 0.878F, 0.761F, 0.290F, 1.0F };
    static constexpr glm::vec4 CURVE{ 0.447F, 0.690F, 0.322F, 1.0F };
    static constexpr glm::vec4 GROUP{ 0.608F, 0.631F, 0.667F, 1.0F };

    /**
     * @return the image a mark's icon resolves to, or an unset one with no way to resolve it
     **/
    v3d::ui::Image image(const Mark& mark) const;

    /**
     * What a walk of the tree is asked for: the filter's words, and the items folded.
     **/
    struct Walk final {
        const scene::Scene& scene;
        const std::vector<std::string>& words;
        const std::set<std::int64_t>& folded;
    };

    /**
     * Add the rows of an item and everything beneath it that is shown.
     * @return whether the item is shown
     **/
    static bool gather(const Walk& walk, const boost::shared_ptr<scene::item::Item>& item, unsigned int depth, std::vector<Row>* into);

    std::map<std::size_t, State> states_;
    Icons icons_;
};

};  // namespace v3d::editor::control
