/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/rib/Handler.h>
#include <api/render/offline/rib/Recorder.h>
#include <api/type/camera/Camera.h>

#include <string>
#include <vector>

#include "Camera.h"
#include "Evaluation.h"
#include "Group.h"
#include "Item.h"
#include "Light.h"
#include "Mesh.h"
#include "RenderSettings.h"
#include "SceneVisitor.h"

#include <boost/shared_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor {

/**
 * Sends a scene to a RIB handler, one request at a time. A rib::Writer makes a RIB file of it,
 * and a renderer's handler renders it. Nothing reads a scene back from RIB.
 *
 * **The camera, the lights, and topology with a placement per mesh.** Each light is a
 * LightSource of RI's standard shader for its type, in world space. RI lights only what is
 * declared after a light, so every light is written before every mesh, whatever the scene's
 * order. The scene's cameras are not written as items: the caller passes the one the export
 * renders from. The scene has no materials, so every surface is RI's "matte", which the lights
 * shade. A scene with no light gets a distant light from the camera, shining the way it looks,
 * so that what it sees is lit.
 *
 * **The picture.** The render settings give the Format, the PixelSamples along each axis and the
 * Hider. A Display names the file the picture is written to, or a "framebuffer" display that
 * leaves it in the renderer.
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
     * @param handler where the requests go, which outlives the export
     **/
    explicit RIBExportVisitor(v3d::render::offline::rib::Handler* handler);

    /**
     * What a render of the export makes: its settings, and where its picture goes.
     **/
    struct Output {
        RenderSettings settings;
        /** The file the picture is written to, or empty to keep it in the renderer. **/
        std::string file;
    };

    /**
     * The picture and the camera, sent before any mesh.
     *
     * The caller passes the camera the export uses, a view's or one made from a scene
     * camera. Its frame aspect should be the picture's, or the picture is stretched.
     *
     * @param camera the view the scene is rendered from
     **/
    void begin(const v3d::type::camera::Camera& camera, const Output& output);

    /**
     * Send the camera's light when the scene had none, then the meshes, and close the world
     * block. A render without this renders nothing.
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
     * The block of an item being visited: the name and local matrix that begin it, and
     * whether it has been sent.
     **/
    struct Block {
        std::string name;
        glm::mat4x4 local{ 1.0F };
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

    v3d::render::offline::rib::Handler* handler_;
    // the meshes, held until every light has been sent
    v3d::render::offline::rib::Recorder body_;
    // the blocks of the item being visited and of each item above it, outermost first
    std::vector<Block> blocks_;
    unsigned int meshes_ = 0;
    unsigned int lights_ = 0;
    unsigned int cameras_ = 0;
    unsigned int groups_ = 0;
    // where the camera is and the way it looks, for the light a scene with none is given
    glm::vec3 eye_{ 0.0F };
    glm::vec3 looks_{ 0.0F, 0.0F, 1.0F };
};

};  // namespace v3d::editor
