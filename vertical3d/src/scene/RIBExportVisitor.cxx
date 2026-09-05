/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "RIBExportVisitor.h"

#include <ostream>
#include <vector>

#include "MeshTopology.h"

namespace v3d::editor {

    RIBExportVisitor::RIBExportVisitor(std::ostream* stream) : stream_(stream) {
    }

    void RIBExportVisitor::matrix(const glm::mat4x4& m) {
        *stream_ << "[";
        for (int column = 0; column < 4; column++) {
            for (int row = 0; row < 4; row++) {
                *stream_ << " " << m[column][row];
            }
        }
        *stream_ << " ]";
    }

    void RIBExportVisitor::begin(const v3d::type::Camera& camera, unsigned int width, unsigned int height) {
        const v3d::type::CameraProfile& profile = camera.profile();
        const glm::vec2 clipping = profile.clipping();

        *stream_ << "##RenderMan RIB-Structure 1.1\n";
        *stream_ << "##Creator Vertical3D\n";
        *stream_ << "version 3.03\n";
        *stream_ << "Format " << width << " " << height << " 1\n";

        /*
            The screen window is written rather than left to the frame aspect, because the
            editor's camera states its aperture as a pixel aspect and an ortho zoom and there
            is no reason to make a reader rederive it.
        */
        if (profile.orthographic()) {
            const float top = profile.orthoZoom();
            const float right = top * profile.pixelAspect();
            *stream_ << "Projection \"orthographic\"\n";
            *stream_ << "ScreenWindow " << -right << " " << right << " " << -top << " " << top << "\n";
        } else {
            *stream_ << "Projection \"perspective\" \"fov\" [" << profile.fov() << "]\n";
            *stream_ << "ScreenWindow " << -profile.pixelAspect() << " " << profile.pixelAspect() << " -1 1\n";
        }
        *stream_ << "Clipping " << clipping.x << " " << clipping.y << "\n";

        // what a scene sets between Projection and WorldBegin is the world to camera
        // transformation, which is what a view matrix is
        *stream_ << "Transform ";
        matrix(camera.view());
        *stream_ << "\n";

        *stream_ << "WorldBegin\n";
    }

    void RIBExportVisitor::end() {
        *stream_ << "WorldEnd\n";
    }

    void RIBExportVisitor::visit(const boost::shared_ptr<v3d::brep::BRep>& mesh) {
        if (!mesh) {
            return;
        }
        meshes_++;

        *stream_ << "AttributeBegin\n";
        *stream_ << "Attribute \"identifier\" \"name\" [\"mesh" << meshes_ << "\"]\n";
        *stream_ << "ConcatTransform ";
        matrix(mesh->matrix());
        *stream_ << "\n";

        const v3d::brep::Index faces = static_cast<v3d::brep::Index>(mesh->faceCount());
        for (v3d::brep::Index face = 0; face < faces; face++) {
            const std::vector<unsigned int> loop = faceLoop(mesh, face);
            if (loop.size() < 3) {
                continue;
            }
            // a half edge names the vertex it ends at, so the loop's segments start where the
            // one before them ended - taking each segment's start walks the face once
            std::vector<glm::vec3> points;
            for (std::size_t index = 0; index < loop.size(); index++) {
                glm::vec3 from(0.0f);
                glm::vec3 to(0.0f);
                if (!loopSegment(mesh, loop, index, &from, &to)) {
                    points.clear();
                    break;
                }
                points.push_back(from);
            }
            if (points.size() < 3) {
                continue;
            }

            *stream_ << "Polygon \"P\" [";
            for (const glm::vec3& point : points) {
                *stream_ << " " << point.x << " " << point.y << " " << point.z;
            }
            *stream_ << " ]\n";
        }

        *stream_ << "AttributeEnd\n";
    }

};  // namespace v3d::editor
