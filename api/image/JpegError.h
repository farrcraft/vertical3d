/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

// jpeglib.h names FILE and size_t without including what declares them
#include <stdio.h>
#include <setjmp.h>
#include <jpeglib.h>

namespace v3d::image {

// jmp_buf is over-aligned, so the struct is padded to suit it. That is the platform's
// requirement rather than something to pack away, and nothing here is written to a file.
#pragma warning(push)
#pragma warning(disable : 4324)
/**
 * The error manager the jpeg reader and writer both install.
 *
 * libjpeg's own error_exit calls exit(), which would end the process over a file that does
 * not decode or a colour space the encoder refuses. This one longjmps back to the setjmp its
 * caller established, which cleans up and returns a failure like any other.
 **/
struct JpegError {
    struct jpeg_error_mgr pub;  // first, so a j_common_ptr's err is also one of these
    jmp_buf setjmp_buffer;
};
#pragma warning(pop)

/**
 * The error_exit a JpegError installs: log through libjpeg's own message routine, then
 * return to the caller's setjmp.
 **/
inline void jpegErrorExit(j_common_ptr cinfo) {
    JpegError* error = reinterpret_cast<JpegError*>(cinfo->err);
    (*cinfo->err->output_message)(cinfo);
    longjmp(error->setjmp_buffer, 1);
}

};  // namespace v3d::image
