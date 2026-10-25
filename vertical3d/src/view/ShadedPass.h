/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/brep/BRep.h>
#include <api/render/realtime/DeviceContext.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/vulkan/memory/Mesh.h>
#include <vertical3d/src/scene/Scene.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <utility>

#include "Triangulation.h"
#include "ViewPort.h"

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

/**
 * The scene's meshes as shaded triangles, drawn into a view's pass before its lines.
 *
 * Each mesh is triangulated and uploaded once for each shading a view asks for, and uploaded
 * again only when its geometry's change count has moved. A mesh's placement is not part of
 * what is uploaded: it reaches the shader in a push constant, so moving a mesh uploads nothing.
 *
 * The front of a triangle and its back are drawn separately, each in the fill its view's
 * display gives that side, unless the display's cull leaves the side out. Every surface is
 * drawn at a depth bias that pushes it back, so a view's wireframe, drawn after it at the same
 * depth, stays in front of it. Where the device cannot draw a triangle as its edges or its
 * corners, a side asked for that way is drawn filled.
 **/
class ShadedPass final {
 public:
    /**
     * The colour a mesh's surface is drawn in, before it is lit.
     **/
    static constexpr glm::vec4 surface{ 0.62F, 0.64F, 0.68F, 1.0F };

    /**
     * Build the six pipelines, one for each side and each fill, against the context's colour
     * and depth formats.
     *
     * @throw std::runtime_error if a pipeline cannot be built
     **/
    explicit ShadedPass(const boost::shared_ptr<v3d::render::realtime::DeviceContext>& context);

    ~ShadedPass();
    ShadedPass(const ShadedPass&) = delete;
    ShadedPass& operator=(const ShadedPass&) = delete;

    /**
     * Draw every mesh of a scene into a pass where the scene's evaluation puts it, as a view's
     * display says. The pass gets the depth bias the surfaces are drawn at. Anything submitted
     * to the pass after this draws over the surfaces.
     **/
    void submit(const Scene& scene, const ViewPort::Display& display, v3d::render::realtime::Pass* pass);

    /**
     * Let go of every upload no submit has drawn since the last call, so a mesh that has left
     * the scene frees its buffers once the frames that drew it are done. Called once a frame,
     * after every view has submitted.
     **/
    void collect();

    /**
     * @return how many uploads are held
     **/
    std::size_t uploaded() const noexcept;

    /**
     * @return how many times a mesh has been triangulated and uploaded, which a mesh whose
     *         geometry has not changed is not
     **/
    std::size_t built() const noexcept;

    /**
     * @return whether a side asked for as points or lines is drawn that way, rather than filled
     **/
    bool nonSolid() const noexcept;

 private:
    /**
     * One mesh's triangles on the device, and the change count they were built at.
     **/
    struct Upload {
        boost::weak_ptr<v3d::brep::BRep> geometry;
        std::uint64_t changes = 0;
        boost::shared_ptr<v3d::render::realtime::vulkan::memory::Mesh> mesh;
        bool drawn = false;
    };

    /**
     * The upload for a mesh in a shading, built or rebuilt as its geometry needs.
     *
     * @return null for a mesh with no triangles
     **/
    const Upload* upload(const boost::shared_ptr<v3d::brep::BRep>& geometry, Shading shading);

    /**
     * Destroy an upload once the frames that may still read it are done.
     **/
    void retire(Upload* upload);

    boost::shared_ptr<v3d::render::realtime::DeviceContext> context_;
    // by side, front then back, and by fill, in the order ViewPort::Fill names them
    std::array<std::array<v3d::render::realtime::PipelineHandle, 3>, 2> pipelines_;
    bool nonSolid_;
    std::size_t built_ = 0;
    std::map<std::pair<const v3d::brep::BRep*, Shading>, Upload> uploads_;
};

};  // namespace v3d::editor
