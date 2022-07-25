/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Jpeg.h"

#include <jpeglib.h>

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

        // Initialize JPEG parameters.
        // we don't yet know the input file's color space, but we need to provide some value for jpeg_set_defaults() to work.
        cinfo.in_color_space = JCS_RGB;  /// arbitrary guess

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

        // Process data
        unsigned char* data = img->data();
        unsigned int num_scanlines = 1;
        unsigned int bytes_width = img->width() * static_cast<int>(img->format());
        data += bytes_width * (cinfo.image_height - 1);
        while (cinfo.next_scanline < cinfo.image_height) {
            jpeg_write_scanlines(&cinfo, &data, num_scanlines);
            data -= bytes_width;
        }

        // Finish compression and release memory
        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);

        return true;
    }

};  // namespace v3d::image::writer
