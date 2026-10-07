/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include <glm/vec2.hpp>

namespace v3d::render::offline {

/**
 * The five pixel filters RI names.
 **/
enum class Filter {
    Box,
    Triangle,
    CatmullRom,
    Gaussian,
    Sinc
};

/**
 * The filter a RIB PixelFilter names: "box", "triangle", "catmull-rom", "gaussian" or "sinc".
 *
 * @return false, leaving filter untouched, for any other name
 **/
bool filterNamed(const std::string & name, Filter * filter);

/**
 * A pixel filter's weight at an offset from the pixel centre, with RI's formulas.
 *
 * @param offset from the centre, in pixels
 * @param width the filter's whole extent along each axis, in pixels; every filter is zero
 *        outside it, including the catmull-rom and sinc ones whose RI formulas ignore it
 **/
float filter(Filter kind, const glm::vec2 & offset, const glm::vec2 & width);

/** The largest picture side a renderer accepts, in pixels. **/
constexpr unsigned int largestResolution = 65536;

/**
 * Whether a picture side is a whole count from 1 to largestResolution. A side outside that
 * range is refused wherever a resolution enters the renderer, because a framebuffer of it
 * cannot be made.
 **/
bool resolution(float side);

/**
 * What one side of a Format asks for. RI reads a side of zero or less as the renderer's
 * default for that side, and this returns it as 0. A side that resolution() accepts is that
 * count, with its fraction dropped. Every other side is refused: one that is not finite, one
 * between 0 and 1, and one above largestResolution.
 **/
std::optional<uint32_t> formatSide(float side);

/**
 * Whether a filter width is a positive finite number on each axis. A width that is not would
 * place a sample in no pixel, or at a pixel index that is not defined, so it is refused
 * wherever a width enters the renderer.
 **/
bool filterWidth(const glm::vec2 & width);

/** The most samples a pixel takes along one axis. **/
constexpr unsigned int maximumSamples = 256;

/**
 * How many samples a PixelSamples rate requests along one axis. RI takes a float. A rate below
 * one, or one that is not a number, takes one sample, and a rate above maximumSamples takes
 * maximumSamples.
 **/
unsigned int sampleCount(float rate);

/**
 * How a frame is sampled: what PixelSamples, PixelFilter, PixelVariance, Shutter and
 * DepthOfField requested. Every field starts at the RI default.
 **/
struct Sampling final {
    glm::uvec2 samples { 2, 2 };
    Filter filter { Filter::Gaussian };
    glm::vec2 width { 2.0f, 2.0f };
    /** 0 means no adaptive sampling. **/
    float variance { 0.0f };
    /** When the shutter opens and closes. **/
    glm::vec2 shutter { 0.0f, 0.0f };
    float fstop { 0.0f };
    float focalLength { 0.0f };
    float focalDistance { 0.0f };

    /**
     * Whether the lens is a pinhole and nothing is out of focus. RI writes a pinhole as an
     * infinite fstop, and an fstop of 0 or a lens with no length is one too.
     **/
    bool pinhole() const;

    /**
     * The lens's radius, focalLength / (2 * fstop), in camera space units. Zero for a pinhole.
     **/
    float lensRadius() const;
};

};  // namespace v3d::render::offline
