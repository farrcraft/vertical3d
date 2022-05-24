/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <stdint.h>

namespace v3d::image
{

#pragma pack(push, 2)

	struct bmp_rgb_quad
	{
		unsigned char blue_;
		unsigned char green_;
		unsigned char red_;
		unsigned char reserved_;
	};

	struct bmp_info_header
	{
		uint32_t		size_;
		int32_t			width_;
		int32_t			height_;
		unsigned short	planes_;
		unsigned short	bits_;
		uint32_t		compression_;
		uint32_t		imageSize_;
		int32_t			xppm_;
		int32_t			yppm_;
		uint32_t		used_;
		uint32_t		important_;
	};

	struct bmp_file_header
	{
		unsigned short	type_;
		uint32_t		size_;
		unsigned short	reserved1_;
		unsigned short	reserved2_;
		uint32_t		offset_;
	};

#pragma pack(pop)

};
