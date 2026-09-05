/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <iosfwd>
#include <string>

#include "SceneVisitor.h"

#include "../../../api/type/Camera.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

    /**
     * Writes a scene out as RIB, one way, per ADR-0023. Nothing reads it back.
     *
     * **Topology and a placement per mesh, and nothing else.** The editor's Scene has no
     * lights and no materials, so what comes out renders in one flat colour until the scene
     * model grows them.
     *
     * A face is written as it stands, which is one Polygon per face. RI says a polygon is
     * planar and convex and does no checking, and moya dices a polygon's first four vertices
     * and drops the rest, so a face with more than four vertices renders as a quad.
     **/
    class RIBExportVisitor final : public SceneVisitor {
     public:
        /**
         * @param stream where the RIB goes - nothing is closed or flushed here
         **/
        explicit RIBExportVisitor(std::ostream* stream);

        /**
         * The file header and the camera, written before any mesh.
         *
         * The scene holds no camera - the views own theirs per ADR-0013 - so the caller says
         * which one the export means.
         *
         * @param camera the view the scene is rendered from
         * @param width the image size to ask the renderer for
         **/
        void begin(const v3d::type::Camera& camera, unsigned int width, unsigned int height);

        /**
         * Close the world block. A file without this renders nothing.
         **/
        void end();

        /**
         * One mesh: its placement, then a Polygon per face.
         **/
        void visit(const boost::shared_ptr<v3d::brep::BRep>& mesh) override;

     private:
        /**
         * Sixteen floats in the order RIB wants them, which is the order glm stores them:
         * both write the translation last, so a transpose here would move the object to
         * where its axes point instead.
         **/
        void matrix(const glm::mat4x4& m);

        std::ostream* stream_;
        unsigned int meshes_ = 0;
    };

};  // namespace v3d::editor
