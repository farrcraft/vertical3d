/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Jpeg.h"

#include <api/image/Image.h>
#include <api/image/Reader.h>
#include <api/image/format/JpegError.h>
#include <api/log/Logger.h>

#include <csetjmp>
#include <cstddef>

namespace v3d::image::reader {
/**
 **/
Jpeg::Jpeg(const boost::shared_ptr<v3d::log::Logger>& logger) : Reader(logger) {
}

boost::shared_ptr<Image> Jpeg::read(const unsigned char* encoded, std::size_t size) {
    boost::shared_ptr<Image> empty_ptr;
    if (encoded == nullptr || size == 0) {
        return empty_ptr;
    }

    struct jpeg_decompress_struct cinfo;

    format::JpegError jerr;
    // We set up the normal JPEG error routines, then override error_exit.
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = format::jpegErrorExit;
    // longjmp does not destroy anything constructed after the setjmp point, so every
    // object below that owns memory is declared above it. The decoder signals failure by
    // longjmping out of any of the jpeg_* calls that follow.
    boost::shared_ptr<Image> img;
    // Establish the setjmp return context for jpegErrorExit to use.
    // C4611 flags the mix of setjmp with C++ object destruction, which the declaration
    // above satisfies: no owning object is constructed after this point.
#pragma warning(push)
#pragma warning(disable : 4611)
    if (setjmp(jerr.setjmp_buffer)) {
        /* The decoder signalled an error. Destroy the decompress object and fail.
            * The input is the caller's buffer, so there is no file to close.
            */
        jpeg_destroy_decompress(&cinfo);
        return empty_ptr;
    }
#pragma warning(pop)
    // Initialize the JPEG decompression object
    jpeg_create_decompress(&cinfo);

    // Specify data source for decompression. jpeg_mem_src does not copy the buffer. The
    // buffer is the caller's and outlives this call, so it outlives the decode.
    // unsigned long is jpeg_mem_src's own parameter type, not a choice made here
    jpeg_mem_src(&cinfo, encoded, static_cast<unsigned long>(size));  // NOLINT(runtime/int)

    // Read file header, set default decompression parameters
    jpeg_read_header(&cinfo, TRUE);

    // Image is three bytes a pixel and the loop below copies three, so the decoder is
    // asked for RGB whatever the file holds. A greyscale jpeg decoded in its own colour
    // space yields one component per pixel, and reading three out of that row walks off
    // the end of it. Set before calc_output_dimensions, which sizes the scanline buffer.
    cinfo.out_color_space = JCS_RGB;

    jpeg_calc_output_dimensions(&cinfo);

    // Create decompressor output buffer.
    const JDIMENSION row_width = cinfo.output_width * cinfo.output_components;
    JSAMPARRAY buffer;
    JDIMENSION buffer_height;
    buffer = (*cinfo.mem->alloc_sarray)
        ((j_common_ptr)&cinfo, JPOOL_IMAGE, row_width, (JDIMENSION)1);
    buffer_height = 1;

    // Start the decompressor. output_scanline then counts the scanlines read
    (void)jpeg_start_decompress(&cinfo);

    img.reset(new Image(glm::uvec2(cinfo.image_width, cinfo.image_height), 24));
    unsigned char* data = img->data();

    // Process data. A jpeg stores its scanlines top down and so does Image, so scanline
    // i is row i of the buffer.
    JDIMENSION num_scanlines = 0;
    unsigned int row = 0;
    size_t index = 0;
    while (cinfo.output_scanline < cinfo.output_height) {
        num_scanlines = jpeg_read_scanlines(&cinfo, buffer, buffer_height);

        index = static_cast<size_t>(row) * 3 * cinfo.output_width;
        for (unsigned int i = 0; i < cinfo.output_width; i++) {
            const size_t sample = static_cast<size_t>(i) * cinfo.output_components;
            data[index] = buffer[0][sample];
            data[index + 1] = buffer[0][sample + 1];
            data[index + 2] = buffer[0][sample + 2];
            index += 3;
        }
        row++;
    }

    // Finish decompression and release memory.
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    return img;
}

};  // namespace v3d::image::reader
