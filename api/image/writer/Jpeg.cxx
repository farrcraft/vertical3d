/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Jpeg.h"

#include <jpeglib.h>

#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

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

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    // open the file
    FILE* fp;
    errno_t err = fopen_s(&fp, static_cast<std::string>(filename).c_str(), "wb");
    if (err != 0) {
        return false;
    }

    // Initialize the JPEG compression object with default error handling.
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    // Initialize JPEG parameters. The colour space has to be set before jpeg_set_defaults(),
    // which reads it to decide the rest - including how many components a scanline has.
    cinfo.in_color_space = img->format() == Image::Format::Grey ? JCS_GRAYSCALE : JCS_RGB;

    jpeg_set_defaults(&cinfo);

    cinfo.input_components = static_cast<int>(img->format());
    cinfo.data_precision = img->bpp() / static_cast<int>(img->format());
    cinfo.image_width = img->width();
    cinfo.image_height = img->height();

    // Now that we know input colorspace, fix colorspace-dependent defaults
    jpeg_default_colorspace(&cinfo);

    // Specify data destination for compression
    jpeg_stdio_dest(&cinfo, fp);

    // Start compressor
    jpeg_start_compress(&cinfo, TRUE);

    // Process data. Both the file and Image are top down, so the scanlines go out in the
    // order they are in.
    unsigned char* data = img->data();
    unsigned int num_scanlines = 1;
    unsigned int bytes_width = img->width() * static_cast<int>(img->format());
    while (cinfo.next_scanline < cinfo.image_height) {
        jpeg_write_scanlines(&cinfo, &data, num_scanlines);
        data += bytes_width;
    }

    // Finish compression and release memory
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    // without this the handle leaks and, worse, the last of the image sits in the stdio
    // buffer - anything that reads the file back before the process exits gets a
    // truncated jpeg
    fclose(fp);

    return true;
}

};  // namespace v3d::image::writer
