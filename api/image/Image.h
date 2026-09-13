/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <stdint.h>

namespace v3d::image {
/**
 * a simple image class. 
 **/
class Image {
 public:
        /**
         * The format of the image (the number of channels)
         *
         * The value is the channel count, which is what a writer divides bpp by to get the
         * bits in a channel. Grey is one channel: a texture atlas packed at depth 1 and a
         * Font2D bitmap are both that, so it is a format the tree makes rather than one
         * held open for later.
         */
        enum class Format {
            Grey = 1,
            RGB = 3,
            RGBA = 4
        };

        Image();
        /**
         * Constructor
         * @param w image width
         * @param h image height
         * @param b bits per pixel
         */
        Image(uint32_t w, uint32_t h, uint8_t b);
        /**
         * Constructor
         * @param len length of the image data in bytes
         */
        explicit Image(uint64_t len);
        virtual ~Image();

        /**
         * An image owns its buffer and frees it, so copying one would free it twice. There
         * is no deep copy here because nothing wants an image by value - a consumer holds a
         * boost::shared_ptr<Image> - and the copy that is actually useful, a rectangle of
         * one image as another, is crop().
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
        /**
         * Set the number of bits per pixel in the image
         * @param bits the number of bits per pixel
         */
        void bpp(uint8_t bits);
        /**
         * Set the width of the image
         * @param w the image width
         */
        void width(unsigned int w);
        /**
         * Set the height of the image
         * @param h the image height
         */
        void height(unsigned int h);

        Format format() const;

        unsigned char & operator[] (unsigned int i);
        unsigned char operator[] (unsigned int i) const;

 private:
        unsigned char * data_;  // Data (Up To 32 Bits)
        uint8_t bpp_;  // Color Depth In Bits Per Pixel
        uint32_t width_;
        uint32_t height_;
        // initialized here rather than only in the constructors, so that a depth no format
        // describes - a raw buffer has none at all - still leaves this a definite value
        Format format_ = Format::RGB;
};

};  // namespace v3d::image
