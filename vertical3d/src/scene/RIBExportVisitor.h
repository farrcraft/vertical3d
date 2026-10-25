/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/camera/Camera.h>

#include <iosfwd>
#include <sstream>
#include <string>
#include <vector>

#include "Camera.h"
#include "Evaluation.h"
#include "Group.h"
#include "Item.h"
#include "Light.h"
#include "Mesh.h"
#include "SceneVisitor.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 * Writes a scene out as RIB. This is export only: nothing reads it back.
 *
 * **The camera, the lights, and topology with a placement per mesh.** Each light is a
 * LightSource of RI's standard shader for its type, in world space. RI lights only what is
 * declared after a light, so every light is written before every mesh, whatever the scene's
 * order. The scene's cameras are not written as items: the caller passes the one the export
 * renders from. The scene has no materials, so what comes out renders in one flat colour.
 *
 * **The hierarchy is kept.** Each item is an AttributeBegin block holding its local matrix,
 * and the blocks of the items beneath it nest inside. A block is written only when a mesh is
 * inside it, so a group with no mesh beneath it, a camera and a light write none. A light's
 * own LightSource is written at world level from its world matrix, because RI turns off a
 * light declared in a block when that block ends.
 *
 * Each face is written as it stands, as one Polygon. RI requires a polygon to be planar and
 * convex, and does no checking. moya dices a polygon's first four vertices and drops the
 * rest, so a face with more than four vertices renders as a quad.
 **/
class RIBExportVisitor final : public SceneVisitor {
 public:
    /**
     * @param stream where the RIB goes. Nothing here closes or flushes it
     **/
    explicit RIBExportVisitor(std::ostream* stream);

    /**
     * The file header and the camera, written before any mesh.
     *
     * The caller passes the camera the export uses, a view's or one made from a scene
     * camera.
     *
     * @param camera the view the scene is rendered from
     * @param width the image size to ask the renderer for
     **/
    void begin(const v3d::type::camera::Camera& camera, unsigned int width, unsigned int height);

    /**
     * Write the meshes and close the world block. A file without this renders nothing.
     **/
    void end();

    /**
     * One mesh: its block with its local matrix, then a Polygon per face. It is held until
     * end(), so that it comes after every light.
     **/
    void visit(const boost::shared_ptr<Mesh>& item, const Placed& placed) override;

    /**
     * A camera is not written. A light is written at once, as a LightSource with "from"
     * where it is and, for a distant or spot light, "to" one unit along its +z axis. A group
     * has only its block.
     **/
    void visit(const boost::shared_ptr<Camera>& camera, const Placed& placed) override;
    void visit(const boost::shared_ptr<Light>& light, const Placed& placed) override;
    void visit(const boost::shared_ptr<Group>& group, const Placed& placed) override;

    /**
     * Close the item's block, if a mesh inside it opened it.
     **/
    void leave(const boost::shared_ptr<Item>& item) override;

 private:
    /**
     * The block of an item being visited: the lines that begin it, and whether they have
     * been written.
     **/
    struct Block {
        std::string begin;
        bool written = false;
    };

    /**
     * Start an item's block, which is written once a mesh is found inside it.
     **/
    void enter(const std::string& name, const glm::mat4x4& local);

    /**
     * Write every block not yet written, outermost first, so a mesh can be written inside.
     **/
    void open();

    /**
     * Sixteen floats in the order RIB expects, which is also the order glm stores them.
     * Both put the translation last, so a transpose here would move the object to where
     * its axes point instead.
     **/
    static void matrix(std::ostream* out, const glm::mat4x4& transform);

    std::ostream* stream_;
    // the meshes, held until every light has been written
    std::ostringstream body_;
    // the blocks of the item being visited and of each item above it, outermost first
    std::vector<Block> blocks_;
    unsigned int meshes_ = 0;
    unsigned int lights_ = 0;
    unsigned int cameras_ = 0;
    unsigned int groups_ = 0;
};

};  // namespace v3d::editor
