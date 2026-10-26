/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/animation/Curve.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "Evaluation.h"
#include "Item.h"
#include "Mesh.h"
#include "RenderSettings.h"
#include "SceneVisitor.h"
#include "Timeline.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 * A set of items of any kind, in the order the scene holds them.
 **/
using Items = std::vector<boost::shared_ptr<Item>>;

/**
 * A set of mesh items, in the order the scene holds them.
 **/
using Meshes = std::vector<boost::shared_ptr<Mesh>>;

/**
 * The curves of one item, each by the path of the property it drives.
 **/
using Curves = std::map<std::string, v3d::type::animation::Curve>;

/**
 * What the editor is editing: the items of one document, of every kind, in one list.
 *
 * Its cameras are part of the document. They are not the views' cameras: the views own
 * theirs, and engine::CameraProfiles owns the table they are built from.
 *
 * Items are found by the id their dag::Node base carries rather than by position, so a
 * selection outlives an insertion or a removal.
 *
 * The scene also gives each item its document id. Ids are never reused within a document, so
 * the scene keeps the next one to give, and a saved document keeps it too.
 *
 * **The hierarchy.** Any item may be the parent of others, and each item has at most one
 * parent, so the items form a set of trees. The scene holds each link, naming both ends by
 * document id, and every link has both ends in the scene. An item's placement is local to its
 * parent. evaluate() composes the world matrices, and every reader takes placement from it.
 *
 * **Animation.** The scene holds the curves that drive its items' properties, each by its
 * item's document id and the property's path, and the document's timeline. pose() sets every
 * keyed property to its curve's value at a frame. It writes into the items, so whatever is
 * edited after it holds until the next pose.
 **/
class Scene final {
 public:
    /**
     * Append an item, and give it the next document id if it has none.
     *
     * @return the id of the item, or 0 for an empty pointer. A selection names an item by
     *         this id.
     **/
    unsigned int add(const boost::shared_ptr<Item>& item);

    /**
     * Take an item out of the scene, and everything beneath it. The links among them and their
     * curves go too, so adding the items back does not bring back the hierarchy or the keys.
     *
     * @return whether an item of that id was there to remove
     **/
    bool remove(unsigned int id);

    /**
     * Put one item beneath another. The child keeps its local placement, so it moves to
     * wherever that places it under the new parent.
     *
     * @param parent the new parent, or 0 to leave the child with none
     * @return false, changing nothing, when either item is not in the scene, or when the
     *         parent is the child or beneath it
     **/
    bool parent(unsigned int child, unsigned int parent);

    /**
     * @return the item's parent, or an empty pointer when it has none or is not in the scene
     **/
    boost::shared_ptr<Item> parent(unsigned int child) const;

    /**
     * @return the items directly beneath one, in the scene's order. An id of 0 gives the
     *         items that have no parent
     **/
    Items children(unsigned int id) const;

    /**
     * @return the item's parent, that item's parent, and so on up to an item with none. It is
     *         empty for an item with no parent or not in the scene
     **/
    Items ancestors(unsigned int id) const;

    /**
     * @return the items of a set that are not beneath any other item of it, in the set's
     *         order. Moving each of these moves the whole set once
     **/
    Items uppermost(const Items& items) const;

    /**
     * @return the item's curves, which are none for an item with no keys or not in the scene
     **/
    Curves curves(unsigned int id) const;

    /**
     * Set the curve that drives one of an item's properties. An empty curve removes it.
     *
     * @return false, changing nothing, when the item is not in the scene or its kind does not
     *         declare the property
     **/
    bool curve(unsigned int id, const std::string& path, const v3d::type::animation::Curve& curve);

    /**
     * @return every frame any of the items has a key on, rising, each once
     **/
    std::vector<float> keyedFrames(const Items& items) const;

    /**
     * Set every property that has a curve to the curve's value at a frame. A property with no
     * curve keeps its value.
     **/
    void pose(float frame);

    /**
     * The document's frame rate, range and current frame.
     **/
    const Timeline& timeline() const noexcept;
    void timeline(const Timeline& timeline) noexcept;

    /**
     * How the document's pictures are rendered.
     **/
    const RenderSettings& renderSettings() const noexcept;
    void renderSettings(const RenderSettings& settings) noexcept;

    /**
     * Compose where every item is: its parent's world matrix times its own local matrix, each
     * parent before its children.
     **/
    Evaluation evaluate() const;

    /**
     * @return the item of that id, or an empty pointer
     **/
    boost::shared_ptr<Item> item(unsigned int id) const;

    /**
     * @return the mesh item of that id, or an empty pointer when there is none or the item
     *         of that id is another kind
     **/
    boost::shared_ptr<Mesh> mesh(unsigned int id) const;

    /**
     * @return every selected item, of every kind, in insertion order
     **/
    Items selection() const;

    /**
     * @return every selected item of one kind, in insertion order. A mesh's component
     *         selection lives inside it, and may span several meshes.
     **/
    template <typename T>
    std::vector<boost::shared_ptr<T>> selected() const {
        std::vector<boost::shared_ptr<T>> found;
        for (const boost::shared_ptr<Item>& item : items_) {
            const boost::shared_ptr<T> kind = as<T>(item);
            if (kind && kind->selected()) {
                found.push_back(kind);
            }
        }
        return found;
    }

    /**
     * @return how many items the scene holds, of every kind
     **/
    std::size_t count() const noexcept;

    /**
     * Drop everything, the hierarchy and the curves with it, as closing a document does. The
     * next document id goes back to 1, and the timeline and the render settings to their
     * defaults.
     **/
    void clear() noexcept;

    /**
     * The document id the next item without one is given. Reading a document sets it to the
     * document's own, which is above every id the document has held.
     **/
    std::int64_t nextDocumentId() const noexcept;
    void nextDocumentId(std::int64_t id) noexcept;

    /**
     * @return every item, of every kind, in insertion order
     **/
    const Items& items() const noexcept;

    /**
     * Walk the items depth first, each through the visit() for its kind, with where one
     * evaluation of the scene puts it. Each parent comes before its children, and siblings
     * come in insertion order.
     **/
    void accept(SceneVisitor* visitor) const;

    /**
     * Clear the selection of every item, and of every component of every mesh.
     **/
    void deselect() noexcept;

    /**
     * Clear the component selection of every mesh, and leave the items selected. A select
     * mask change does this, because components of one kind mean nothing to an operation
     * working on another.
     **/
    void deselectComponents() noexcept;

 private:
    /**
     * The item of a document id, or an empty pointer.
     **/
    boost::shared_ptr<Item> document(std::int64_t id) const;

    Items items_;
    // each child's parent, both named by document id
    std::unordered_map<std::int64_t, std::int64_t> parents_;
    // each animated item's curves, by its document id
    std::map<std::int64_t, Curves> curves_;
    Timeline timeline_;
    RenderSettings renderSettings_;
    std::int64_t nextDocumentId_ = 1;
};

};  // namespace v3d::editor
