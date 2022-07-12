/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Jpeg.h"

#include <jpeglib.h>

#include <fstream>
#include <iostream>
#include <cstdio>
// for errno
#include <cerrno>
// for strerror
#include <cstring>
// for setjmp/longjmp used in jpeg error handling
#include <csetjmp>
#include <string>


namespace v3d::image::reader {
    /**
     **/
    Jpeg::Jpeg(const boost::shared_ptr<v3d::core::Logger>& logger) : Reader(logger) {
    }

    // JPEG library error handling
    struct my_error_mgr {
        struct jpeg_error_mgr pub;  // "public" fields
        jmp_buf setjmp_buffer;  // for return to caller
    };

    typedef struct my_error_mgr* my_error_ptr;

    /*
        * Here's the routine that will replace the standard error_exit method:
        */
    METHODDEF(void) my_error_exit(j_common_ptr cinfo) {
        // cinfo->err really points to a my_error_mgr struct, so coerce pointer
        my_error_ptr myerr = (my_error_ptr)cinfo->err;

        // Always display the message.
        // We could postpone this until after returning, if we chose.
        (*cinfo->err->output_message) (cinfo);

        // Return control to the setjmp point
        longjmp(myerr->setjmp_buffer, 1);
    }

    boost::shared_ptr<Image> Jpeg::read(std::string_view filename) {
        LOG_DEBUG(logger_) << "JPEGReader::read - Reading jpeg file [" << filename << "]";

        boost::shared_ptr<Image> empty_ptr;
        struct jpeg_decompress_struct cinfo;

        // open the file
        FILE* fp;
        errno = 0;
        errno_t err = fopen_s(&fp, static_cast<std::string>(filename).c_str(), "rb");
        if (err != 0) {
            LOG_DEBUG(logger_) << "JPEGReader::read - failed opening file [" << filename << "] with errno [" <<
                strerror(errno) << "]";
            return empty_ptr;
        }

        struct my_error_mgr jerr;
        // We set up the normal JPEG error routines, then override error_exit.
        cinfo.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = my_error_exit;
        // Establish the setjmp return context for my_error_exit to use.
        if (setjmp(jerr.setjmp_buffer)) {
            /* If we get here, the JPEG code has signaled an error.
                * We need to clean up the JPEG object, close the input file, and return.
                */
            jpeg_destroy_decompress(&cinfo);
            fclose(fp);
            return empty_ptr;
        }
        // Initialize the JPEG decompression object
        jpeg_create_decompress(&cinfo);

        // Specify data source for decompression
        jpeg_stdio_src(&cinfo, fp);

        // Read file header, set default decompression parameters
        jpeg_read_header(&cinfo, TRUE);

        jpeg_calc_output_dimensions(&cinfo);

        // Create decompressor output buffer.
        JDIMENSION row_width = cinfo.output_width * cinfo.output_components;
        JSAMPARRAY buffer;
        JDIMENSION buffer_height;
        buffer = (*cinfo.mem->alloc_sarray)
            ((j_common_ptr)&cinfo, JPOOL_IMAGE, row_width, (JDIMENSION)1);
        buffer_height = 1;

        // Adjust default decompression parameters by re-parsing the options
        (void)jpeg_start_decompress(&cinfo);

        boost::shared_ptr<Image> img(new Image(cinfo.image_width, cinfo.image_height, 24));
        unsigned char* data = img->data();

        // Process data
        JDIMENSION num_scanlines = 0;
        int row = cinfo.output_height - 1;
        unsigned int index = 0;
        while (cinfo.output_scanline < cinfo.output_height) {
            num_scanlines = jpeg_read_scanlines(&cinfo, buffer, buffer_height);

            index = row * cinfo.output_components * cinfo.output_width;
            for (unsigned int i = 0; i < cinfo.output_width; i++) {
                data[index] = buffer[0][(i * cinfo.output_components)];
                data[index + 1] = buffer[0][(i * cinfo.output_components) + 1];
                data[index + 2] = buffer[0][(i * cinfo.output_components) + 2];
                index += 3;
            }
            row--;
        }

        // Finish decompression and release memory.
        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);

        fclose(fp);
        return img;
    }

};  // namespace v3d::image::reader
