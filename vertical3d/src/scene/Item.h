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
 * One thing the scene holds: a mesh, a camera or a light.
 *
 * Every kind has two ids, a placement from its dag::Transform base, and an object selection
 * flag. The id from its dag::Node base is unique within one run of the editor, and a selection
 * names an item by it. The document id is unique within one document and is saved with it, so
 * anything saved that refers to an item names it by that.
 *
 * An item cannot be copied, because a copy would share its id.
 **/
class Item : public v3d::dag::Node, public v3d::dag::Transform {
 public:
    /**
     * Which class an item is. as() checks it before it casts.
     **/
    enum class Kind : std::uint8_t { Mesh, Camera, Light };

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
     * Whether the item as a whole is selected, which is object mode selection. A mesh's
     * component selection is the flag each of its vertices, half edges and faces carries.
     **/
    bool selected() const noexcept;
    void selected(bool selected) noexcept;

 private:
    bool selected_;
    std::int64_t documentId_;
};

/**
 * An item as one kind, or an empty pointer when it is another kind or there is no item.
 *
 * T is Mesh, Camera or Light, each of which names its own kind as T::KIND.
 **/
template <typename T>
boost::shared_ptr<T> as(const boost::shared_ptr<Item>& item) {
    if (!item || item->kind() != T::KIND) {
        return boost::shared_ptr<T>();
    }
    return boost::static_pointer_cast<T>(item);
}

};  // namespace v3d::editor
