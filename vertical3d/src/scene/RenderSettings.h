/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>

#include <glm/vec2.hpp>

namespace v3d::editor {

/**
 * How a document's pictures are rendered: their size, how many samples each pixel takes along
 * each axis, and which hider decides what is seen.
 **/
struct RenderSettings final {
    enum class Hider : std::uint8_t {
        Hidden,    /**< reyes: dice into micropolygons and hide them per bucket **/
        RayTrace   /**< trace rays from the camera through each pixel **/
    };

    static constexpr unsigned int DEFAULT_WIDTH = 640;
    static constexpr unsigned int DEFAULT_HEIGHT = 480;
    /** The longest side the editor renders, a quarter of the longest moya takes. **/
    static constexpr unsigned int LARGEST_SIDE = 16384;
    /** The most samples along each axis of a pixel. **/
    static constexpr unsigned int MOST_SAMPLES = 16;

    glm::uvec2 size{ DEFAULT_WIDTH, DEFAULT_HEIGHT };
    unsigned int samples = 2;
    Hider hider = Hider::Hidden;

    /**
     * @return whether both sides are between 1 and LARGEST_SIDE, and the samples between 1
     *         and MOST_SAMPLES
     **/
    bool valid() const noexcept {
        return size.x >= 1 && size.y >= 1 && size.x <= LARGEST_SIDE && size.y <= LARGEST_SIDE &&
            samples >= 1 && samples <= MOST_SAMPLES;
    }

    bool operator==(const RenderSettings& other) const = default;
};

};  // namespace v3d::editor
