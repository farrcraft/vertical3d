/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <stdint.h>

#include <cstdint>

#include <glm/vec2.hpp>

namespace v3d::image {
/**
 * a simple image class. 
 **/
class Image {
 public:
        /**
         * The format of the image (the number of channels)
         *
         * The value is the channel count. A writer divides bpp by it to get the bits in a
         * channel. Grey is one channel; a texture atlas packed at depth 1 produces it.
         */
        enum class Format : std::uint8_t {
            Grey = 1,
            RGB = 3,
            RGBA = 4
        };

        Image();
        /**
         * Constructor
         * @param dimensions image width and height
         * @param bitsPerPixel bits per pixel
         */
        Image(const glm::uvec2& dimensions, uint8_t bitsPerPixel);
        virtual ~Image();

        /**
         * An image owns its buffer and frees it, so a copy would free it twice. Copying is
         * deleted, not made deep. A consumer holds a boost::shared_ptr<Image>, and crop()
         * copies a rectangle of one image into a new one.
         */
        Image(const Image &) = delete;
        Image & operator=(const Image &) = delete;

        /**
         * Get the image data
         * @return a pointer to the image data
         */
        unsigned char * data();
        /**
         * Get the number of bits per pixel in the image
         * @return number of bits per pixel
         */
        uint8_t bpp() const;
        /**
         * Get the image width
         * @return the image width
         */
        uint32_t width() const;
        /**
         * Get the image height
         * @return the image height
         */
        uint32_t height() const;

        Format format() const;

        unsigned char & operator[] (unsigned int index);
        unsigned char operator[] (unsigned int index) const;

 private:
        unsigned char * data_;  // Data (Up To 32 Bits)
        uint8_t bpp_;  // Color Depth In Bits Per Pixel
        uint32_t width_;
        uint32_t height_;
        // every constructor also sets this. formatOf() gives RGB for a depth no format
        // describes, so the value is always one of the three
        Format format_ = Format::RGB;
};

};  // namespace v3d::image
