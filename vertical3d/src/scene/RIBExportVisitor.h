/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/camera/Camera.h>

#include <iosfwd>
#include <string>

#include "Camera.h"
#include "Light.h"
#include "Mesh.h"
#include "SceneVisitor.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 * Writes a scene out as RIB. This is export only: nothing reads it back.
 *
 * **Topology and a placement per mesh, and nothing else.** The scene's cameras and lights are
 * not written, and it has no materials, so what comes out renders in one flat colour.
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
     * The caller passes the camera the export uses, which is a view's.
     *
     * @param camera the view the scene is rendered from
     * @param width the image size to ask the renderer for
     **/
    void begin(const v3d::type::camera::Camera& camera, unsigned int width, unsigned int height);

    /**
     * Close the world block. A file without this renders nothing.
     **/
    void end();

    /**
     * One mesh: its placement, then a Polygon per face.
     **/
    void visit(const boost::shared_ptr<Mesh>& item) override;

    /**
     * Cameras and lights are not written.
     **/
    void visit(const boost::shared_ptr<Camera>& camera) override;
    void visit(const boost::shared_ptr<Light>& light) override;

 private:
    /**
     * Sixteen floats in the order RIB expects, which is also the order glm stores them.
     * Both put the translation last, so a transpose here would move the object to where
     * its axes point instead.
     **/
    void matrix(const glm::mat4x4& transform);

    std::ostream* stream_;
    unsigned int meshes_ = 0;
};

};  // namespace v3d::editor
