/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/dag/Node.h>
#include <api/dag/Transform.h>

#include <cstdint>

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 * One thing the scene holds: a mesh, a camera, a light or a group.
 *
 * Every kind has two ids, a placement from its dag::Transform base, and an object selection
 * flag. The id from its dag::Node base is unique within one run of the editor, and a selection
 * names an item by it. The document id is unique within one document and is saved with it, so
 * anything saved that refers to an item names it by that.
 *
 * The placement is local: it is measured in the space of the item's parent, which the scene
 * holds. Where the item is in the world comes from the scene's evaluation.
 *
 * An item cannot be copied, because a copy would share its id.
 **/
class Item : public v3d::dag::Node, public v3d::dag::Transform {
 public:
    /**
     * Which class an item is. as() checks it before it casts.
     **/
    enum class Kind : std::uint8_t { Mesh, Camera, Light, Group };

    Item();
    ~Item() override;

    virtual Kind kind() const noexcept = 0;

    /**
     * The item's id within its document, or 0 before the scene first assigns one. The scene
     * assigns it when the item is first added, and the item keeps it after a removal, so an
     * undone creation brings it back unchanged.
     **/
    std::int64_t documentId() const noexcept;
    void documentId(std::int64_t id) noexcept;

    /**
     * A number that moves whenever anything a document saves of the item changes: its
     * placement, what its kind holds, and a mesh's geometry. It never goes down, so a caller
     * that kept it can tell whether the item has changed since. Its selection is not saved, and
     * does not move it.
     **/
    virtual std::uint64_t revision() const noexcept;

    /**
     * Whether the item as a whole is selected, which is object mode selection. A mesh's
     * component selection is the flag each of its vertices, half edges and faces carries.
     **/
    bool selected() const noexcept;
    void selected(bool selected) noexcept;

 protected:
    /**
     * Count a change to what a kind holds, which a subclass's setters make.
     **/
    void changed() noexcept;

 private:
    bool selected_;
    std::int64_t documentId_;
    std::uint64_t changes_ = 0;
};

/**
 * An item as one kind, or an empty pointer when it is another kind or there is no item.
 *
 * T is Mesh, Camera, Light or Group, each of which names its own kind as T::KIND.
 **/
template <typename T>
boost::shared_ptr<T> as(const boost::shared_ptr<Item>& item) {
    if (!item || item->kind() != T::KIND) {
        return boost::shared_ptr<T>();
    }
    return boost::static_pointer_cast<T>(item);
}

};  // namespace v3d::editor
