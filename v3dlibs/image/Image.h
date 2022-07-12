/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

namespace v3d::image {
	/**
	 * a simple image class. 
	 **/
	class Image {
		public:
			/**
			 * The format of the image (the number of channels)
			 */
			enum class Format {
				RGB = 3,
				RGBA = 4
			};

			Image();
			/**
			 * Constructor
			 * @param w image width
			 * @param h image height
			 * @param b bits per pixel
			 */
			Image(unsigned int w, unsigned int h, unsigned int b);
			/**
			 * Constructor
			 * @param len length of the image data in bytes
			 */
			Image(unsigned long len);
			virtual ~Image();

			/**
			 * Get the image data
			 * @return a pointer to the image data
			 */
			unsigned char * data();
			/**
			 * Get the number of bits per pixel in the image
			 * @return number of bits per pixel
			 */
			unsigned int bpp() const;
			/**
			 * Get the image width
			 * @return the image width
			 */
			unsigned int width() const;
			/**
			 * Get the image height
			 * @return the image height
			 */
			unsigned int height() const;
			/**
			 * Set the number of bits per pixel in the image
			 * @param bits the number of bits per pixel
			 */
			void bpp(unsigned int bits);
			/**
			 * Set the width of the image
			 * @param w the image width
			 */
			void width(unsigned int w);
			/**
			 * Set the height of the image
			 * @param h the image height
			 */
			void height(unsigned int h);

			Format format() const;

			unsigned char & operator[] (unsigned int i);
			unsigned char operator[] (unsigned int i) const;

		private:
			unsigned char * data_;	// Data (Up To 32 Bits)
			unsigned int bpp_;		// Color Depth In Bits Per Pixel
			unsigned int width_;
			unsigned int height_;
			Format format_;
	};

};
