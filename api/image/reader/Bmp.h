/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/image/Reader.h>
#include <api/image/format/BmpFileHeader.h>
#include <api/image/format/BmpInfoHeader.h>
#include <api/image/format/BmpRgbQuad.h>
#include <api/log/Logger.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::image::reader {
/**
 **/
class Bmp final : public v3d::image::Reader {
 public:
        Bmp(const boost::shared_ptr<v3d::log::Logger> & logger);
        ~Bmp() = default;

        // the path form is the base's, written in terms of this one
    using v3d::image::Reader::read;

    boost::shared_ptr<Image> read(const unsigned char* encoded, std::size_t length) override;

 private:
    /**
     * Where each channel sits in a packed 16 or 32 bit pixel.
     **/
    struct Masks final {
        uint32_t red = 0;
        uint32_t green = 0;
        uint32_t blue = 0;
        uint32_t alpha = 0;
    };

    /**
     * Where a bmp's rows are and how they are laid out.
     **/
    struct Layout final {
        unsigned int bits = 0;
        uint64_t columns = 0;
        uint64_t rows = 0;
        uint64_t pad = 0;       /**< the bytes a row takes on disk, padded to a dword boundary **/
        bool bottomUp = true;   /**< a positive height stores the last row first, which is the usual way **/
        unsigned int channels = 3;
    };

    // an uncompressed 16 bit file has five bits a channel
    static const Masks RGB555_MASKS;

    // and an uncompressed 32 bit file has eight, with alpha in the top byte
    static const Masks ARGB8888_MASKS;

    // each mask after the info header is one word, in the order red, green, blue, alpha
    static constexpr std::size_t MASK_SIZE = sizeof(uint32_t);

    static constexpr uint32_t bitfields = 3;

    static void logHeaders(const boost::shared_ptr<v3d::log::Logger>& logger, const format::bmp_file_header& fheader,
        const format::bmp_info_header& iheader, int num_colors);

    /**
     * One channel of a packed pixel, picked out by its mask and widened to a byte. A mask of
     * five bits gives 0 to 31, which is spread over 0 to 255 so that full is 255.
     **/
    static unsigned char channel(uint32_t pixel, uint32_t mask);

    static uint32_t readWord(const unsigned char* at);

    /**
     * Whether the reader has a conversion for this file: 8, 16, 24 or 32 bits, uncompressed or, at
     * 16 and 32 bits, packed by masks, and with pixels to read.
     **/
    static bool supported(const format::bmp_info_header& header, const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     * Where each channel of a 16 or 32 bit pixel is: the standard places for an uncompressed file,
     * or the masks the file gives. A 40 byte header is followed by three masks, and a longer one
     * holds them and an alpha mask too.
     **/
    static bool readMasks(const unsigned char* encoded, std::size_t length, const format::bmp_info_header& header, Masks* masks);

    /**
     * An 8 bit file's colour table, which follows the info header whatever that header's length.
     * A file says how many entries it uses, and none means all of them.
     **/
    static bool readPalette(const unsigned char* encoded, std::size_t length, const format::bmp_info_header& header,
        std::vector<format::bmp_rgb_quad>* colors);

    /**
     * One pixel, as rgb or rgba.
     *
     * @return whether its alpha is anything but zero
     **/
    static bool convertPixel(const unsigned char* src, uint64_t column, const Layout& layout, const Masks& masks,
        const std::vector<format::bmp_rgb_quad>& colors, unsigned char* out);

    /**
     * Every row, top down whatever order the file stored them in. A 32 bit file whose alpha is zero
     * everywhere did not use it, as most writers do not, and is made opaque rather than invisible.
     **/
    static void convertRows(const unsigned char* pixels, const Layout& layout, const Masks& masks,
        const std::vector<format::bmp_rgb_quad>& colors, unsigned char* data);
};

};  // namespace v3d::image::reader
