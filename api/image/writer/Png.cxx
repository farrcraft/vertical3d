/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Png.h"

#include <api/image/File.h>
#include <api/image/Image.h>
#include <api/image/Writer.h>
#include <api/log/Logger.h>

#include <png.h>

#include <csetjmp>
#include <cstddef>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace {

/**
 * What libpng's error and warning handlers report through: the file being written, and the log.
 **/
struct Reporting {
    std::string name;
    v3d::log::Logger* logger = nullptr;
};

void warned(png_structp png, png_const_charp message) {
    const Reporting* reporting = static_cast<const Reporting*>(png_get_error_ptr(png));
    reporting->logger->get()->warn("libpng, writing {}: {}", reporting->name, message);
}

/**
 * Reports the error and returns, and libpng then longjmps to the png_jmpbuf that encode() set.
 **/
void failed(png_structp png, png_const_charp message) {
    const Reporting* reporting = static_cast<const Reporting*>(png_get_error_ptr(png));
    reporting->logger->get()->error("libpng, writing {}: {}", reporting->name, message);
}

/**
 * The picture in the shape libpng's header takes.
 **/
struct Picture {
    png_uint_32 width = 0;
    png_uint_32 height = 0;
    int depth = 8;
    int colour = PNG_COLOR_TYPE_RGB;
    png_color_8 significant = {};
    png_bytepp rows = nullptr;
};

/**
 * Every libpng call that can fail, behind the setjmp that catches the failure. libpng reports an
 * error by longjmp, which skips destructors, so nothing here has one. The caller owns and frees
 * everything.
 *
 * @return false when libpng reported an error
 **/
// C4611 warns of any setjmp in C++. Nothing in encode() has a destructor for a longjmp to skip
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4611)
#endif
bool encode(png_structp png, png_infop info, Picture* picture) {
    if (setjmp(png_jmpbuf(png))) {  // NOLINT(cert-err52-cpp) - libpng reports an error by longjmp
        return false;
    }
    png_set_IHDR(png, info, picture->width, picture->height, picture->depth, picture->colour,
        PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_set_sBIT(png, info, &picture->significant);
    png_write_info(png, info);
    png_write_image(png, picture->rows);
    png_write_end(png, info);
    return true;
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

};  // namespace

namespace v3d::image::writer {
/**
 **/
Png::Png(const boost::shared_ptr<v3d::log::Logger>& logger) : Writer(logger) {
}

/**
 **/
bool Png::write(std::string_view filename, const boost::shared_ptr<Image>& img) {
    if (!img) {
        return false;
    }

    // png_set_sBIT reads whichever members the colour type covers, so the ones this writer does
    // not set have to be zero rather than indeterminate
    Picture picture;
    picture.width = img->width();
    picture.height = img->height();
    const png_byte bytes = static_cast<png_byte>(img->bpp() / static_cast<int>(img->format()));
    picture.depth = bytes;
    if (img->format() == Image::Format::Grey) {
        // a grey png carries its depth in the gray member and has no colour members at all
        picture.colour = PNG_COLOR_TYPE_GRAY;
        picture.significant.gray = bytes;
    } else {
        picture.significant.red = bytes;
        picture.significant.green = bytes;
        picture.significant.blue = bytes;
        if (img->format() == Image::Format::RGBA) {
            picture.colour = PNG_COLOR_TYPE_RGB_ALPHA;
            picture.significant.alpha = bytes;
        }
    }

    // both the file and Image are top down, so the rows go out in the order they are in
    const std::size_t stride = static_cast<std::size_t>(picture.width) * static_cast<std::size_t>(img->format());
    std::vector<png_bytep> rows(picture.height);
    png_bytep data = img->data();
    for (std::size_t row = 0; row < rows.size(); row++) {
        rows[row] = data + (row * stride);
    }
    picture.rows = rows.data();

    FILE* fp = openFile(static_cast<std::string>(filename), "wb");
    if (fp == nullptr) {
        return false;
    }
    // the name lives as long as the write, because libpng's handlers read it
    Reporting reporting{.name = static_cast<std::string>(filename), .logger = logger().get()};
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, &reporting, failed, warned);
    png_infop info = png == nullptr ? nullptr : png_create_info_struct(png);
    bool written = false;
    if (info != nullptr) {
        png_init_io(png, fp);
        written = encode(png, info, &picture);
    }
    png_destroy_write_struct(&png, &info);
    fclose(fp);
    return written;
}

};  // namespace v3d::image::writer
