/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/Transform.h>

#include <cstdint>

#include <boost/shared_ptr.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor {

/**
 * One thing the scene holds: a mesh, a camera, a light, a group or curves.
 *
 * Every kind has two ids, a placement, and an object selection flag. id() is unique within one
 * run of the editor, and a selection names an item by it. The document id is unique within one
 * document and is saved with it, so anything saved that refers to an item names it by that.
 *
 * The placement is local: it is measured in the space of the item's parent, which the scene
 * holds. Where the item is in the world comes from the scene's evaluation.
 *
 * An item cannot be copied, because a copy would share its id.
 **/
class Item {
 public:
    /**
     * Which class an item is. as() checks it before it casts.
     **/
    enum class Kind : std::uint8_t { Mesh, Camera, Light, Group, Curve };

    Item();
    virtual ~Item();
    Item(const Item&) = delete;
    Item& operator=(const Item&) = delete;

    virtual Kind kind() const noexcept = 0;

    /**
     * The item's id within this run of the editor. Each item constructed gets an id no earlier
     * item had, and a later item a larger one.
     **/
    unsigned int id() const noexcept;

    /**
     * The placement, a translation, a rotation and a scale. Each setter moves revision(), a
     * setter that writes what is already there included.
     **/
    const v3d::type::Transform& placement() const noexcept;
    void placement(const v3d::type::Transform& placement) noexcept;

    glm::vec3 translation() const noexcept;
    void translation(const glm::vec3& translation) noexcept;
    glm::quat rotation() const noexcept;
    void rotation(const glm::quat& rotation) noexcept;
    glm::vec3 scale() const noexcept;
    void scale(const glm::vec3& scale) noexcept;

    /**
     * Add an offset to the translation. A drag of the translate manipulator measures a delta,
     * not a destination, so it moves the item through this.
     **/
    void translate(const glm::vec3& offset) noexcept;

    /**
     * @return translation * rotation * scale, the placement as a matrix in the parent's space
     **/
    glm::mat4 matrix() const;

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
    unsigned int id_;
    v3d::type::Transform placement_;
    bool selected_;
    std::int64_t documentId_;
    std::uint64_t changes_ = 0;
};

/**
 * An item as one kind, or an empty pointer when it is another kind or there is no item.
 *
 * T is Mesh, Camera, Light, Group or Curve, each of which names its own kind as T::KIND.
 **/
template <typename T>
boost::shared_ptr<T> as(const boost::shared_ptr<Item>& item) {
    if (!item || item->kind() != T::KIND) {
        return boost::shared_ptr<T>();
    }
    return boost::static_pointer_cast<T>(item);
}

};  // namespace v3d::editor
