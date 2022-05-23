/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "PNGWriter.h"

#include <png.h>

#include <iostream>
#include <cstdio>

using namespace v3d::image::writer;

PNGWriter::PNGWriter() {
}

PNGWriter::~PNGWriter() {
}

static void
pngtest_warning(png_structp png_ptr, png_const_charp message) {
	PNG_CONST char *name = "UNKNOWN (ERROR!)";
	png_voidp error_ptr = NULL;
	if (png_ptr != NULL)
		error_ptr = png_get_error_ptr(png_ptr);
	if (error_ptr != NULL)
		name = reinterpret_cast<char*>(error_ptr);
	std::cout << name << ": libpng warning: " << message << std::endl;
}

/* This is the default error handling function.  Note that replacements for
 * this function MUST NOT RETURN, or the program will likely crash.  This
 * function is used by default, or if the program supplies NULL for the
 * error function pointer in png_set_error_fn().
 */
static void
pngtest_error(png_structp png_ptr, png_const_charp message) {
   pngtest_warning(png_ptr, message);
   /* We can return because png_error calls the default handler, which is
    * actually OK in this case. */
}

bool PNGWriter::write(const std::string & filename, const boost::shared_ptr<Image> & img) {
	// open the file
	FILE * fp;
	if ((fp = fopen(filename.c_str(), "wb")) == 0)
	{
		return false;
	}

	png_structp png_ptr;
	png_infop info_ptr;

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (png_ptr == NULL)
	{
		fclose(fp);
		return false;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
		fclose(fp);
		png_destroy_write_struct(&png_ptr,  NULL);
		return false;
	}

	png_init_io(png_ptr, fp);

	png_set_error_fn(png_ptr, (png_voidp)filename.c_str(), pngtest_error,
       pngtest_warning);

	png_color_8 sig_bit;
	int bytes = img->bpp() / img->format();
	sig_bit.red = bytes;
	sig_bit.green = bytes;
	sig_bit.blue = bytes;

	int color_type;
	if (img->format() == Image::FORMAT_RGB) {
		color_type = PNG_COLOR_TYPE_RGB;
	}
	else if (img->format() == Image::FORMAT_RGBA)
	{
		color_type = PNG_COLOR_TYPE_RGB_ALPHA;
		/// if the image has an alpha channel then
		sig_bit.alpha = bytes;
	}
	png_set_IHDR(png_ptr, info_ptr, img->width(), img->height(), bytes, color_type,
				 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	// significant bit chunk
	png_set_sBIT(png_ptr, info_ptr, &sig_bit);

	// write header info
	png_write_info(png_ptr, info_ptr);

   /* Shift the pixels up to a legal bit depth and fill in
    * as appropriate to correctly scale the image.
    */
//	png_set_shift(png_ptr, &sig_bit);

	/* pack pixels into bytes */
//	png_set_packing(png_ptr);

	png_uint_32 k, height, bytes_per_pixel, width, j;
	height = img->height();
	bytes_per_pixel = img->format();
	width = img->width();
	png_byte ** row_pointers = 0;
	row_pointers = new png_bytep[height];

	if (height > PNG_UINT_32_MAX/sizeof(png_bytep))
		png_error (png_ptr, "Image is too tall to process in memory");

	png_bytep data = img->data();
	j = height - 1;
	for (k = 0; k < height; k++) {
		row_pointers[j] = data + k*width*bytes_per_pixel;
		j--;
	}

	png_write_image(png_ptr, row_pointers);

	png_write_end(png_ptr, info_ptr);
	// clean up after the write, and free any memory allocated */
	png_destroy_write_struct(&png_ptr, &info_ptr);

	delete [] row_pointers;

	fclose(fp);

	return true;
}
