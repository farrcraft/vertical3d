/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Jpeg.h"

#include <api/image/Image.h>
#include <api/image/Writer.h>
#include <api/image/format/JpegError.h>
#include <api/log/Logger.h>

#include <array>
#include <csetjmp>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace {

/**
 * Where libjpeg puts what it encodes. It fills the chunk, and each full chunk, and the part
 * of the last one it filled, go on the end of the string.
 **/
struct Destination {
    jpeg_destination_mgr manager;  // first, so the library's pointer to it is one of these
    std::array<JOCTET, 4096> chunk;
    std::string* bytes;
};

Destination* destination(j_compress_ptr cinfo) {
    return reinterpret_cast<Destination*>(cinfo->dest);
}

void started(j_compress_ptr cinfo) {
    Destination* into = destination(cinfo);
    into->manager.next_output_byte = into->chunk.data();
    into->manager.free_in_buffer = into->chunk.size();
}

boolean filled(j_compress_ptr cinfo) {
    Destination* into = destination(cinfo);
    into->bytes->append(reinterpret_cast<const char*>(into->chunk.data()), into->chunk.size());
    started(cinfo);
    return TRUE;
}

void finished(j_compress_ptr cinfo) {
    Destination* into = destination(cinfo);
    into->bytes->append(reinterpret_cast<const char*>(into->chunk.data()), into->chunk.size() - into->manager.free_in_buffer);
}

/**
 * Every libjpeg call that can fail, behind the setjmp that catches the failure. libjpeg reports
 * an error by longjmp, which skips destructors, so nothing here has one. The caller owns and
 * frees everything, and destroys the compressor whichever way this returns.
 *
 * @param row a scanline's worth of room, for an image whose alpha has to be dropped
 * @return false when libjpeg reported an error
 **/
// C4611 warns of any setjmp in C++. Nothing in compress() has a destructor for a longjmp to skip
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4611)
#endif
bool compress(jpeg_compress_struct* cinfo, v3d::image::format::JpegError* error, Destination* into,
    v3d::image::Image& img, unsigned char* row) {
    if (setjmp(error->setjmp_buffer)) {  // NOLINT(cert-err52-cpp) - libjpeg reports an error by longjmp
        return false;
    }
    jpeg_create_compress(cinfo);
    // The colour space has to be set before jpeg_set_defaults(), which reads it to decide
    // the rest. That includes how many components a scanline has.
    const bool alpha = img.format() == v3d::image::Image::Format::RGBA;
    cinfo->in_color_space = img.format() == v3d::image::Image::Format::Grey ? JCS_GRAYSCALE : JCS_RGB;
    jpeg_set_defaults(cinfo);
    cinfo->input_components = alpha ? 3 : static_cast<int>(img.format());
    cinfo->data_precision = img.bpp() / static_cast<int>(img.format());
    cinfo->image_width = img.width();
    cinfo->image_height = img.height();
    // Now that we know input colorspace, fix colorspace-dependent defaults
    jpeg_default_colorspace(cinfo);

    into->manager.init_destination = started;
    into->manager.empty_output_buffer = filled;
    into->manager.term_destination = finished;
    cinfo->dest = &into->manager;
    jpeg_start_compress(cinfo, TRUE);

    // Both a jpeg and Image are top down, so the scanlines go out in the order they are in.
    // A jpeg has no alpha, so an RGBA image goes out a row at a time with it dropped
    unsigned char* data = img.data();
    const std::size_t bytes_width = static_cast<std::size_t>(img.width()) * static_cast<int>(img.format());
    while (cinfo->next_scanline < cinfo->image_height) {
        JSAMPROW line = data;
        if (alpha) {
            for (unsigned int column = 0; column < img.width(); ++column) {
                row[(column * 3) + 0] = data[(column * 4) + 0];
                row[(column * 3) + 1] = data[(column * 4) + 1];
                row[(column * 3) + 2] = data[(column * 4) + 2];
            }
            line = row;
        }
        jpeg_write_scanlines(cinfo, &line, 1);
        data += bytes_width;
    }
    jpeg_finish_compress(cinfo);
    return true;
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

};  // namespace

namespace v3d::image::writer {
/**
 **/
Jpeg::Jpeg(const boost::shared_ptr<v3d::log::Logger>& logger) : Writer(logger) {
}

/**
 **/
std::optional<std::string> Jpeg::encode(const boost::shared_ptr<Image>& img) {
    if (!img) {
        return std::nullopt;
    }

    jpeg_compress_struct cinfo{};
    format::JpegError jerr;
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = format::jpegErrorExit;

    std::string encoded;
    Destination into{};
    into.bytes = &encoded;
    std::vector<unsigned char> row(img->format() == Image::Format::RGBA ? static_cast<std::size_t>(img->width()) * 3 : 0);

    const bool compressed = compress(&cinfo, &jerr, &into, *img, row.data());
    jpeg_destroy_compress(&cinfo);
    if (!compressed) {
        logger()->get()->error("the jpeg encoder refused a {} by {} picture", img->width(), img->height());
        return std::nullopt;
    }
    return encoded;
}

};  // namespace v3d::image::writer
