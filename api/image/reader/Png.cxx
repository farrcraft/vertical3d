/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Png.h"

#pragma pack(push, 1)

#include <png.h>

#pragma pack(pop)

#include <csetjmp>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

namespace v3d::image::reader {

namespace {

/**
 * How far into the buffer libpng has read, since it walks rather than seeks.
 **/
struct Cursor final {
    const unsigned char* data;
    std::size_t size;
    std::size_t at;
};

/**
 * libpng's own read, over memory instead of a FILE.
 *
 * A short buffer is an error rather than a short read: png_error longjmps out of the
 * decode, which is what stops the rows below being written from whatever was on the stack.
 **/
void readFromBuffer(png_structp png, png_bytep into, png_size_t wanted) {
    Cursor* cursor = static_cast<Cursor*>(png_get_io_ptr(png));
    if (cursor == nullptr || cursor->at + wanted > cursor->size) {
        png_error(png, "the png ended before it said it would");
        return;
    }
    memcpy(into, cursor->data + cursor->at, wanted);
    cursor->at += wanted;
}

};  // namespace

/**
 **/
Png::Png(const boost::shared_ptr<v3d::log::Logger>& logger) : Reader(logger) {
}

/**
 **/
boost::shared_ptr<Image> Png::read(const unsigned char* encoded, std::size_t size) {
    boost::shared_ptr<Image> empty_ptr;

    // make sure it's really a png file. png_sig_cmp rather than png_check_sig: the same
    // test, and the one that takes the bytes as const
    if (encoded == nullptr || size < 8 || png_sig_cmp(encoded, 0, 8) != 0) {
        return empty_ptr;
    }
    Cursor cursor{ encoded, size, 8 };

    png_structp png_ptr = 0;
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if (!png_ptr) {
        return empty_ptr;
    }

    png_infop info_ptr = 0;
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        return empty_ptr;
    }

    // longjmp does not destroy anything constructed after the setjmp point, so every
    // object below that owns memory is declared above it. Without this libpng's default
    // error handler aborts the process, which a reader pointed at bytes it did not open
    // itself cannot afford - a truncated png is an ordinary thing to be handed.
    boost::shared_ptr<Image> img;
    std::vector<png_bytep> rowpointers;
    // C4611 flags the mix of setjmp with C++ object destruction, which the declarations
    // above satisfy: no owning object is constructed after this point.
#pragma warning(push)
#pragma warning(disable : 4611)
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return empty_ptr;
    }
#pragma warning(pop)

    png_set_read_fn(png_ptr, &cursor, readFromBuffer);

    png_set_sig_bytes(png_ptr, 8);

    // read all PNG info up to image data
    png_read_info(png_ptr, info_ptr);

    // get width, height, bit-depth and color-type
    png_uint_32 width;
    png_uint_32 height;
    int bpp;
    int colors;
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bpp, &colors, 0, 0, 0);

    // convert to 3x8 RGB if necessary
    if (bpp == 16)
        png_set_strip_16(png_ptr);
    if (colors == PNG_COLOR_TYPE_PALETTE)
        png_set_expand(png_ptr);
    if (bpp < 8)
        png_set_expand(png_ptr);
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_expand(png_ptr);
    if (colors == PNG_COLOR_TYPE_GRAY || colors == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png_ptr);

    // after the transformations have been registered update info_ptr data
    png_read_update_info(png_ptr, info_ptr);

    // get width, height and the new bit-depth and color-type
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bpp, &colors, 0, 0, 0);

    // row_bytes is the width x number of channels
    size_t rowbytes;
    uint64_t channels;
    rowbytes = png_get_rowbytes(png_ptr, info_ptr);
    channels = png_get_channels(png_ptr, info_ptr);

    // now we can allocate memory to store the image
    img.reset(new Image(width, height, static_cast<uint8_t>(channels * bpp)));
    unsigned char* data = img->data();

    // set the individual row-pointers to point at the correct offsets. A png file stores
    // its rows top down and so does Image, so row i of the file is row i of the buffer
    rowpointers.resize(height);
    for (unsigned int i = 0; i < height; i++) {
        rowpointers[i] = data + (i * rowbytes);
    }

    // now we can go ahead and just read the whole image
    png_read_image(png_ptr, rowpointers.data());

    // read the additional chunks in the PNG file (not really needed)
    png_read_end(png_ptr, 0);

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    return img;
}

};  // namespace v3d::image::reader
