/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "Item.h"
#include "Mesh.h"
#include "SceneVisitor.h"

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
 * What the editor is editing: the items of one document, of every kind, in one list.
 *
 * Its cameras are part of the document. They are not the views' cameras: the views own
 * theirs, and config::CameraProfiles owns the table they are built from.
 *
 * Items are found by the id their dag::Node base carries rather than by position, so a
 * selection outlives an insertion or a removal.
 *
 * The scene also gives each item its document id. Ids are never reused within a document, so
 * the scene keeps the next one to give, and a saved document keeps it too.
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
     * @return whether an item of that id was there to remove
     **/
    bool remove(unsigned int id);

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
     * Drop everything, as closing a document does. The next document id goes back to 1.
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
     * Walk the items in insertion order, each through the visit() for its kind.
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
    Items items_;
    std::int64_t nextDocumentId_ = 1;
};

};  // namespace v3d::editor
