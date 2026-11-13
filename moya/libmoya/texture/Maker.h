/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/render/offline/film/Sampling.h>
#include <moya/libmoya/texture/Level.h>

#include <cstdint>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/vec2.hpp>

namespace v3d::log {
class Logger;
};  // namespace v3d::log

namespace v3d::moya::colour {
class Config;
};  // namespace v3d::moya::colour

/**
 * The textures the new core reads: tiled, mipmapped OpenEXR files, and the maker that writes them
 * from images.
 **/
namespace v3d::moya::texture {

/** How a texture is read beyond its edge along one axis, as RI's MakeTexture names it. **/
enum class Wrap : std::uint8_t {
    Black,     /**< every texel beyond the edge is zero **/
    Clamp,     /**< a texel beyond the edge is the edge's nearest texel **/
    Periodic   /**< the texture repeats **/
};

/**
 * The wrap mode RI names "black", "clamp" or "periodic".
 *
 * @return false, leaving wrap untouched, for any other name
 **/
bool wrapNamed(const std::string & name, Wrap * wrap);

/** The name RI gives a wrap mode, which wrapNamed() reads back. **/
std::string wrapName(Wrap wrap);

/**
 * How a texture is made from an image: MakeTexture's wrap modes, its filter and the filter's
 * widths, and the colour space the image is read in.
 *
 * - width is the filter's whole extent along s and t, in texels of the level being made, at most
 *   kWidest. A box of width one averages the two by two texels of the level above that one texel
 *   covers.
 * - colourSpace is a colour space's name or role in the configuration. Empty reads the image in
 *   its own space: an EXR file's chromaticities, linear Rec. 709 for an EXR file with none, and
 *   sRGB for any other image. A space that holds data, as Raw does, keeps the image's values.
 **/
struct Making {
    Wrap swrap { Wrap::Black };
    Wrap twrap { Wrap::Black };
    render::offline::film::Filter filter { render::offline::film::Filter::Box };
    glm::vec2 width { 1.0F };
    std::string colourSpace;
};

/** The widest a filter may be along each axis, in texels of the level being made. **/
inline constexpr float kWidest = 64.0F;

/** Whether a filter's widths are each above zero, finite and at most kWidest. **/
bool widthAllowed(const glm::vec2 & width);

/**
 * Every level of a texture, the image first and a texel of one by one last. Each level is half the
 * one above along each side, rounded down and never below one texel, as OpenEXR's mipmaps are.
 *
 * Each texel of a level is the filtered sum of the level above, over the filter's widths about the
 * texel's centre, divided by the filter's weights. A texel the filter reaches beyond the edge is
 * found by the axis's wrap mode, and a black one adds its weight and nothing else. A sum whose
 * weights total zero takes the nearest texel of the level above.
 *
 * @param image the first level
 * @param channels how many values each texel holds, from 1 to 4
 * @throws std::invalid_argument for an image with no texels, values that do not fill it, a count
 *         of channels outside 1 to 4, or a width widthAllowed() refuses
 **/
std::vector<Level> mipmap(const Level & image, std::uint32_t channels, const Making & making);

/**
 * Makes a texture file from an image, as RI's MakeTexture does.
 *
 * The image is an OpenEXR file, or a PNG, JPEG, TGA or BMP file. An EXR file's R, G, B and A
 * channels are read, or its Y and A. Any other image's channels are those its reader gives, each
 * of 8 bits, and a PNG reader gives a grey image as R, G and B.
 *
 * A colour image is converted into ACEScg before its levels are filtered, so they average light.
 * Its texture holds R, G, B and A if the image has an alpha, which is not converted. A grey colour
 * image becomes R, G and B. A data image keeps its values and its channels, Y for one, Y and A for
 * two. An EXR file whose chromaticities name none of output::kLinearIds' spaces is read as linear
 * Rec. 709, and the log says so.
 *
 * The texture is a tiled OpenEXR file of 64 by 64 texel tiles, every level held, compressed by
 * ZIP. Its values are floats when the image's are, and halves otherwise. Its wrapmodes attribute
 * names the s and t modes, as "periodic,clamp", and a colour texture's chromaticities are
 * ACEScg's. The file is written beside the texture first and moved into place, so a reader never
 * sees half of one.
 *
 * @param picture the image, as UTF-8
 * @param texture the texture file written, as UTF-8
 * @param logger the log the image's readers and the maker's warnings write to, never null
 * @throws std::runtime_error when the image cannot be read, the colour space is not in the
 *         configuration, or the texture cannot be written
 * @throws std::invalid_argument for a width mipmap() refuses
 **/
void make(const std::string & picture, const std::string & texture, const Making & making, const colour::Config & config,
    const boost::shared_ptr<log::Logger> & logger);

};  // namespace v3d::moya::texture
