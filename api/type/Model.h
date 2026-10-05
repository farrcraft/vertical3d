/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/animation/Clip.h>

#include <cstdint>
#include <string>
#include <vector>

#include "Skeleton.h"

#include <glm/gtc/type_precision.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::type {

/**
 * Loaded geometry: one interleaved vertex array, the indices into it, and the surface it
 * is drawn with.
 *
 * The third thing in the tree called a mesh, and the one that is neither of the others. A
 * brep::BRep is topology the editor models with, and a render::realtime::vulkan::memory::Mesh is
 * two device buffers; this is what a file on disk turns into on the way from one to the
 * other. It holds no handle and no device type, so a renderer that never opens a window
 * can read one.
 *
 * Everything is merged into a single vertex array and a single index run, so a model is
 * one upload. A part is a range of that index run drawn with one material, so a file whose
 * surfaces differ is one model drawn as a draw per part - ADR-0069.
 **/
class Model final {
 public:
    /**
     * The interleaved attributes, in the order a pipeline declaring this layout expects
     * them. A vertex buffer is bytes and a stride to the device - vulkan::memory::Mesh says the
     * stride is the pipeline's - so this layout is a contract between a loader and
     * whatever pipeline an app writes to draw with it, and not something the device
     * enforces.
     **/
    struct Vertex final {
        glm::vec3 position{ 0.0f, 0.0f, 0.0f };
        glm::vec3 normal{ 0.0f, 0.0f, 0.0f };
        glm::vec2 uv{ 0.0f, 0.0f };
    };

    /**
     * How a model's surface looks: a colour, and the name of the image tinting it.
     *
     * The texture is a name rather than pixels, which is
     * [ADR-0020](../../docs/adr/0020-ui-themes-are-data-apps-load-the-images.md)'s
     * shape - the data names an image and the app resolves it through the asset manager,
     * which is what already knows where assets live and what has been loaded once. A
     * loader that decoded the pixels itself would be a second image pipeline beside
     * api/image.
     **/
    struct Material final {
        glm::vec4 baseColour{ 1.0f, 1.0f, 1.0f, 1.0f };

        /**
         * What the base colour is multiplied by, named the way the model file named it and
         * so relative to wherever that file was. Empty when the surface is a flat colour.
         **/
        std::string baseColourTexture;
    };

    /**
     * Which joints a vertex follows, and how far: the weighted sum of their matrices is what
     * moves it. Four at most, and the weights sum to one.
     **/
    struct Influence final {
        glm::u16vec4 joints{ 0, 0, 0, 0 };  /**< into the skeleton's joints **/
        glm::vec4 weights{ 0.0f };
    };

    /**
     * A range of the index run, drawn with one of the model's materials.
     **/
    struct Part final {
        std::uint32_t firstIndex{ 0 };
        std::uint32_t indexCount{ 0 };
        std::uint32_t material{ 0 };  /**< into materials() **/
    };

    Model();

    std::vector<Vertex>& vertices() noexcept;
    const std::vector<Vertex>& vertices() const noexcept;

    std::vector<std::uint32_t>& indices() noexcept;
    const std::vector<std::uint32_t>& indices() const noexcept;

    std::vector<Material>& materials() noexcept;
    const std::vector<Material>& materials() const noexcept;

    /**
     * What is drawn, in order. Every index is in exactly one part, and a model the loader
     * reads has at least one.
     **/
    std::vector<Part>& parts() noexcept;
    const std::vector<Part>& parts() const noexcept;

    /**
     * The joints the model is bent by, empty for a static model.
     **/
    Skeleton& skeleton() noexcept;
    const Skeleton& skeleton() const noexcept;

    /**
     * An influence per vertex, in the vertices' order, for a model with a skeleton - and
     * empty for one without, which is what keeps a static model's vertices as they were.
     **/
    std::vector<Influence>& influences() noexcept;
    const std::vector<Influence>& influences() const noexcept;

    /**
     * The clips that animate the skeleton, by the names the file gave them. Empty for a model
     * with no skeleton.
     **/
    std::vector<animation::Clip>& clips() noexcept;
    const std::vector<animation::Clip>& clips() const noexcept;

    /**
     * @return the vertex array's size in bytes, which is what a device buffer is made from
     **/
    std::size_t vertexBytes() const noexcept;

    /**
     * @return whether there is any geometry at all
     **/
    bool empty() const noexcept;

 private:
    std::vector<Vertex> vertices_;
    std::vector<std::uint32_t> indices_;
    std::vector<Material> materials_;
    std::vector<Part> parts_;
    Skeleton skeleton_;
    std::vector<Influence> influences_;
    std::vector<animation::Clip> clips_;
};

};  // namespace v3d::type
