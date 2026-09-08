/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Jpeg.h"

#include <jpeglib.h>

#include <cstddef>
// for setjmp/longjmp used in jpeg error handling
#include <csetjmp>
#include <string>


namespace v3d::image::reader {
/**
 **/
Jpeg::Jpeg(const boost::shared_ptr<v3d::log::Logger>& logger) : Reader(logger) {
}

// JPEG library error handling
// jmp_buf is over-aligned, so the struct is padded to suit it. That is the platform's
// requirement rather than something to pack away, and nothing here is written to a file.
#pragma warning(push)
#pragma warning(disable : 4324)
struct my_error_mgr {
    struct jpeg_error_mgr pub;  // "public" fields
    jmp_buf setjmp_buffer;  // for return to caller
};
#pragma warning(pop)

typedef struct my_error_mgr* my_error_ptr;

namespace {

/*
    * Here's the routine that will replace the standard error_exit method:
    */
// libjpeg spells this METHODDEF(void), which is static - the anonymous namespace is what
// gives it internal linkage here, and both together is a redundant static
void my_error_exit(j_common_ptr cinfo) {
    // cinfo->err really points to a my_error_mgr struct, so coerce pointer
    my_error_ptr myerr = (my_error_ptr)cinfo->err;

    // Always display the message.
    // We could postpone this until after returning, if we chose.
    (*cinfo->err->output_message) (cinfo);

    // Return control to the setjmp point
    longjmp(myerr->setjmp_buffer, 1);
}

};  // namespace

boost::shared_ptr<Image> Jpeg::read(const unsigned char* encoded, std::size_t size) {
    boost::shared_ptr<Image> empty_ptr;
    if (encoded == nullptr || size == 0) {
        return empty_ptr;
    }

    struct jpeg_decompress_struct cinfo;

    struct my_error_mgr jerr;
    // We set up the normal JPEG error routines, then override error_exit.
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    // longjmp does not destroy anything constructed after the setjmp point, so every
    // object below that owns memory is declared above it. The decoder signals failure by
    // longjmping out of any of the jpeg_* calls that follow.
    boost::shared_ptr<Image> img;
    // Establish the setjmp return context for my_error_exit to use.
    // C4611 flags the mix of setjmp with C++ object destruction, which the declaration
    // above satisfies: no owning object is constructed after this point.
#pragma warning(push)
#pragma warning(disable : 4611)
    if (setjmp(jerr.setjmp_buffer)) {
        /* If we get here, the JPEG code has signaled an error.
            * We need to clean up the JPEG object, close the input file, and return.
            */
        jpeg_destroy_decompress(&cinfo);
        return empty_ptr;
    }
#pragma warning(pop)
    // Initialize the JPEG decompression object
    jpeg_create_decompress(&cinfo);

    // Specify data source for decompression. The buffer is not copied, so it has to
    // outlive the decode - it is the caller's and outlives the whole call.
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
    JDIMENSION row_width = cinfo.output_width * cinfo.output_components;
    JSAMPARRAY buffer;
    JDIMENSION buffer_height;
    buffer = (*cinfo.mem->alloc_sarray)
        ((j_common_ptr)&cinfo, JPOOL_IMAGE, row_width, (JDIMENSION)1);
    buffer_height = 1;

    // Adjust default decompression parameters by re-parsing the options
    (void)jpeg_start_decompress(&cinfo);

    img.reset(new Image(cinfo.image_width, cinfo.image_height, 24));
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
