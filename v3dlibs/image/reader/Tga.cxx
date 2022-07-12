/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Tga.h"

#include <fstream>
#include <iostream>
#include <cstring>

using namespace v3d::image;
using namespace v3d::image::reader;

boost::shared_ptr<Image> Tga::read(std::string_view filename)
{
	// 2 = uncompressed rgb
	// 3 = uncompressed b&w
	unsigned char rgbTGAheader[12]={0,0,2,0,0,0,0,0,0,0,0,0};// Uncompressed TGA Header
	unsigned char bwTGAheader[12]={0,0,3,0,0,0,0,0,0,0,0,0};// Uncompressed TGA Header
	unsigned char TGAcompare[12];						// Used To Compare TGA Header
	unsigned char header[6];							// First 6 Useful Bytes From The Header

	std::fstream file;
	file.open(static_cast<std::string>(filename).c_str(), std::fstream::in | std::fstream::binary);
	boost::shared_ptr<Image> empty_ptr;
	if (file.fail())
	{
		return empty_ptr;
	}

	std::streamoff length;
	file.seekg(0, std::fstream::end);
	length = file.tellg();
	file.seekg(0, std::fstream::beg);
	if (length < (sizeof(TGAcompare) + sizeof(header)))
	{
		file.close();
		return empty_ptr;
	}

	file.read(reinterpret_cast<char*>(&TGAcompare), sizeof(TGAcompare));
	if (memcmp(rgbTGAheader, TGAcompare, sizeof(rgbTGAheader)) != 0 && memcmp(bwTGAheader, TGAcompare, sizeof(bwTGAheader)))
	{
		file.close();
		return empty_ptr;
	}

	file.read(reinterpret_cast<char*>(&header), sizeof(header));

	unsigned int width  = header[1] * 256 + header[0];	// Determine The TGA Width	(highbyte*256+lowbyte)
	unsigned int height = header[3] * 256 + header[2];	// Determine The TGA Height	(highbyte*256+lowbyte)
	unsigned int bpp = header[4];			// Grab The TGA's Bits Per Pixel (24 or 32)
    
 	if(width <= 0 ||						// Is The Width Less Than Or Equal To Zero
		height	<= 0 ||						// Is The Height Less Than Or Equal To Zero
		(header[4] != 24 && header[4] != 32 && header[4] != 8))	// 8/24/32 bit?
	{
		file.close();
		return empty_ptr;
	}

	boost::shared_ptr<Image> img(new Image(width, height, bpp));

	unsigned int bytespp = bpp / 8;
	unsigned int size = width * height * bytespp;
	unsigned char * data = img->data();
	file.read(reinterpret_cast<char*>(data), size);

	if (file.fail())
	{
		file.close();
		return empty_ptr;
	}

	if (bytespp >= 3)
	{
		unsigned int temp;
		for (unsigned int i = 0; i < int(size); i += bytespp)
		{										// Swaps The 1st And 3rd Bytes ('R'ed and 'B'lue)
			temp = data[i];
			data[i] = data[i + 2];
			data[i + 2] = temp;
		}
	}
	file.close();
	return img;
}
