/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

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

/** The most samples a pixel takes along one axis. **/
constexpr unsigned int maximumSamples = 256;

/**
 * How many samples a PixelSamples rate requests along one axis. RI takes a float. A rate
 * below one, or one that is not a number, takes one sample, and a rate above
 * maximumSamples takes maximumSamples.
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
