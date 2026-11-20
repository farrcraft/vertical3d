/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/rib/Handler.h>
#include <api/rib/Parameters.h>
#include <api/rib/Recorder.h>
#include <api/rib/requests/ScreenWindow.h>
#include <api/type/camera/Camera.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/scene/item/Camera.h>
#include <vertical3d/src/scene/Evaluation.h>
#include <vertical3d/src/scene/item/Group.h>
#include <vertical3d/src/scene/item/Item.h>
#include <vertical3d/src/scene/item/Light.h>
#include <vertical3d/src/scene/item/Mesh.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Visitor.h>

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor::render {

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
 * **The picture.** The render settings give the Format and the PixelSamples along each axis. No
 * Hider is written, since moya renders by path tracing alone. A Display names the file the
 * picture is written to, or a "framebuffer" display that leaves it in the renderer.
 *
 * **The hierarchy is kept.** Each item is an AttributeBegin block holding its local matrix and
 * an identifier of its name, and the blocks of the items beneath it nest inside. A light, which
 * is written at world level, has its identifier set there before it. A block is written only
 * when a mesh or a curve that renders is inside it, so a group with neither beneath it, a camera
 * and a light write none. A light's
 * own LightSource is written at world level from its world matrix, because RI turns off a
 * light declared in a block when that block ends.
 *
 * **A curve item renders only when its style says so.** Its curves are converted to basis
 * curves, and each run of curves with the same basis and wrap is one Curves request of that
 * many curves, after a Basis request when they are cubic. Each has the style's width as its
 * "constantwidth".
 *
 * **A mesh is one PointsPolygons**: every vertex of the mesh, and the triangles each face is
 * cut into, since RI draws a polygon only when it is flat and convex.
 **/
class RIBExportVisitor final : public scene::Visitor {
 public:
    /**
     * @param handler where the requests go, which outlives the export
     **/
    explicit RIBExportVisitor(v3d::rib::Handler* handler);

    /**
     * What a render of the export makes: its settings, and where its picture goes.
     **/
    struct Output {
        scene::RenderSettings settings;
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
     * The screen window a camera is sent with: its ortho zoom by its pixel aspect for an
     * orthographic camera, and the pixel aspect by one for a perspective one.
     **/
    static v3d::rib::requests::ScreenWindow screen(const v3d::type::camera::Profile& profile);

    /** The name of the light a scene with no light of its own is given. **/
    static constexpr const char* CAMERA_LIGHT = "camera light";

    /** The surface every mesh and curve takes: RI's "matte", which the lights shade. **/
    static constexpr const char* SURFACE = "matte";

    /** The name of the display a picture kept in the renderer is sent to. **/
    static constexpr const char* FRAMEBUFFER = "vertical3d";

    /**
     * How many Bezier segments each span of a NURBS curve that is weighted, or above cubic, is
     * approximated by. Four keep a circle within 0.05% of its radius.
     **/
    static constexpr unsigned int APPROXIMATION_PIECES = 4;

    /**
     * Send the camera's light when the scene had none, then the meshes, and close the world
     * block. A render without this renders nothing.
     **/
    void end();

    /**
     * One mesh: its block with its local matrix, then one PointsPolygons. It is held until
     * end(), so that it comes after every light.
     **/
    void visit(const boost::shared_ptr<scene::item::Mesh>& item, const scene::Placed& placed) override;

    /**
     * A camera is not written. A light is written at once, as a LightSource with "from"
     * where it is and, for a distant or spot light, "to" one unit along its +z axis. A group
     * has only its block.
     **/
    void visit(const boost::shared_ptr<scene::item::Camera>& camera, const scene::Placed& placed, const scene::item::Camera::Lens& lens) override;
    void visit(const boost::shared_ptr<scene::item::Light>& light, const scene::Placed& placed, const scene::item::Light::Emission& emission) override;
    void visit(const boost::shared_ptr<scene::item::Group>& group, const scene::Placed& placed) override;
    void visit(const boost::shared_ptr<scene::item::Curve>& curve, const scene::Placed& placed) override;

    /**
     * Close the item's block, if a mesh inside it opened it.
     **/
    void leave(const boost::shared_ptr<scene::item::Item>& item) override;

 private:
    /** The version of RIB the export is written in. **/
    static constexpr float RIB_VERSION = 3.03F;
    /** An identifier attribute that names what follows. **/
    static v3d::rib::ParameterList identifier(const std::string& name);
    static std::vector<float> floats(const glm::vec3& value);

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

    v3d::rib::Handler* handler_;
    // the meshes, held until every light has been sent
    v3d::rib::Recorder body_;
    // the blocks of the item being visited and of each item above it, outermost first
    std::vector<Block> blocks_;
    unsigned int lights_ = 0;
    // where the camera is and the way it looks, for the light a scene with none is given
    glm::vec3 eye_{ 0.0F };
    glm::vec3 looks_{ 0.0F, 0.0F, 1.0F };
};

};  // namespace v3d::editor::render
