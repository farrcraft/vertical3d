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
 * A brep::BRep is the topology the editor models with, and a
 * render::realtime::Mesh is a pair of device buffers. A Model is what a file on
 * disk is loaded into before it is uploaded. It holds no handle and no device type, so a
 * renderer that never opens a window can read one.
 *
 * All geometry is merged into a single vertex array and a single index run, so a model is
 * one upload. A part is a range of that index run drawn with one material. A file with
 * several surfaces is one model, drawn with one draw per part.
 **/
class Model final {
 public:
    /**
     * The interleaved attributes, in the order a pipeline declaring this layout expects
     * them. The device sees only bytes and a stride, and the pipeline sets the stride. A
     * loader and the pipeline an app draws with must agree on this layout, and nothing
     * checks that they do.
     **/
    struct Vertex final {
        glm::vec3 position{ 0.0F, 0.0F, 0.0F };
        glm::vec3 normal{ 0.0F, 0.0F, 0.0F };
        glm::vec2 uv{ 0.0F, 0.0F };
    };

    /**
     * How a model's surface looks: a colour, and the name of the image tinting it.
     *
     * The texture is a name rather than pixels. The app resolves the name through the
     * asset manager, which holds where assets live and what is already loaded.
     **/
    struct Material final {
        glm::vec4 baseColour{ 1.0F, 1.0F, 1.0F, 1.0F };

        /**
         * The image the base colour is multiplied by, named as the model file names it. The
         * name is relative to the model file's directory. Empty when the surface is a flat
         * colour.
         **/
        std::string baseColourTexture;
    };

    /**
     * Which joints a vertex follows, and how far. The weighted sum of their matrices moves
     * the vertex. There are four at most, and the weights sum to one.
     **/
    struct Influence final {
        glm::u16vec4 joints{ 0, 0, 0, 0 };  /**< into the skeleton's joints **/
        glm::vec4 weights{ 0.0F };
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
     * An influence per vertex, in the vertices' order, for a model with a skeleton. Empty
     * for a model without one, so a static model carries no skinning data.
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
     * @return the vertex array's size in bytes, which is the size of its device buffer
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
