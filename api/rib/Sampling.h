/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include <glm/vec2.hpp>

namespace v3d::rib {

/**
 * The pixel filters RI names.
 **/
enum class Filter : std::uint8_t {
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
 * The name a RIB PixelFilter gives a filter, which filterNamed() reads back.
 **/
std::string filterName(Filter filter);

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

};  // namespace v3d::rib
