/*
	Copyright (C) 2001-2004 by Josh Farr
	merkaba_at_quantumfish_dot_com

	This file is part of Vertical|3D.

	Vertical|3D is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Vertical|3D is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Vertical|3D; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef INCLUDED_V3D_IMAGE_H
#define INCLUDED_V3D_IMAGE_H

#include "IOBase.h"

namespace v3D
{

	class Image : public IOBase
	{
		public:
			Image();
			Image(unsigned int width, unsigned int height, unsigned int bpp);
			~Image();

			typedef enum 
			{
				IMAGE_FORMAT_RGB 	= 3, // values are number of components
				IMAGE_FORMAT_RGBA 	= 4
			} FormatType;

			unsigned char * allocate(unsigned int width, unsigned int height, unsigned int bpp);
			unsigned char * allocate(void);
			void			destroy(void);
			unsigned char * image(void);
			unsigned int	width(void) const;
			unsigned int	height(void) const;
			unsigned int	bpp(void) const;
			//unsigned int	components(void) const;
			void			format(FormatType format);
			FormatType		format(void) const;
			

		private:
			unsigned int	_width;
			unsigned int	_height;
			unsigned int	_bpp;
			unsigned char * _image;
			FormatType		_format;
	};

	typedef Image * ImagePtr;

}; // end namespace v3D

#endif // INCLUDED_V3D_IMAGE_H
