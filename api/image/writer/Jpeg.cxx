/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Jpeg.h"

#include <api/image/JpegError.h>

#include <cstdio>
#include <string>
#include <vector>

namespace v3d::image::writer {
/**
 **/
Jpeg::Jpeg(const boost::shared_ptr<v3d::log::Logger>& logger) : Writer(logger) {
}

/**
 **/
bool Jpeg::write(std::string_view filename, const boost::shared_ptr<Image>& img) {
    if (!img) {
        return false;
    }
    const std::string path(filename);

    // open the file
    FILE* fp;
    errno_t err = fopen_s(&fp, path.c_str(), "wb");
    if (err != 0) {
        return false;
    }

    struct jpeg_compress_struct cinfo;
    JpegError jerr;
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = jpegErrorExit;

    // a jpeg has no alpha, so an RGBA image goes out a row at a time with it dropped. The
    // row is declared here because longjmp destroys nothing constructed after the setjmp
    const bool alpha = img->format() == Image::Format::RGBA;
    std::vector<unsigned char> row(alpha ? static_cast<std::size_t>(img->width()) * 3 : 0);

    // C4611 flags the mix of setjmp with C++ object destruction, which the declarations
    // above satisfy: no owning object is constructed after this point.
#pragma warning(push)
#pragma warning(disable : 4611)
    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_compress(&cinfo);
        fclose(fp);
        // what was written is a fragment of a jpeg, which is worse than no file
        std::remove(path.c_str());
        logger_->get()->error("JpegWriter::write - the encoder refused {}", path);
        return false;
    }
#pragma warning(pop)

    jpeg_create_compress(&cinfo);

    // The colour space has to be set before jpeg_set_defaults(), which reads it to decide
    // the rest - including how many components a scanline has.
    cinfo.in_color_space = img->format() == Image::Format::Grey ? JCS_GRAYSCALE : JCS_RGB;

    jpeg_set_defaults(&cinfo);

    cinfo.input_components = alpha ? 3 : static_cast<int>(img->format());
    cinfo.data_precision = img->bpp() / static_cast<int>(img->format());
    cinfo.image_width = img->width();
    cinfo.image_height = img->height();

    // Now that we know input colorspace, fix colorspace-dependent defaults
    jpeg_default_colorspace(&cinfo);

    jpeg_stdio_dest(&cinfo, fp);
    jpeg_start_compress(&cinfo, TRUE);

    // Both the file and Image are top down, so the scanlines go out in the order they are in.
    unsigned char* data = img->data();
    const std::size_t bytes_width = static_cast<std::size_t>(img->width()) * static_cast<int>(img->format());
    while (cinfo.next_scanline < cinfo.image_height) {
        JSAMPROW line = data;
        if (alpha) {
            for (unsigned int column = 0; column < img->width(); ++column) {
                row[column * 3 + 0] = data[column * 4 + 0];
                row[column * 3 + 1] = data[column * 4 + 1];
                row[column * 3 + 2] = data[column * 4 + 2];
            }
            line = row.data();
        }
        jpeg_write_scanlines(&cinfo, &line, 1);
        data += bytes_width;
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    // without this the handle leaks and, worse, the last of the image sits in the stdio
    // buffer - anything that reads the file back before the process exits gets a
    // truncated jpeg
    fclose(fp);

    return true;
}

};  // namespace v3d::image::writer
