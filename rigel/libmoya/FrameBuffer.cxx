/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
 **/

#include <iostream>
#include <cassert>

#include "FrameBuffer.h"

using namespace v3D::Moya;

FrameBuffer::FrameBuffer(unsigned int bucketSize[2], unsigned int imageSize[2]) 
/* : 
			_buckets(boost::extents[imageSize[0] / bucketSize[0]][imageSize[1] / bucketSize[1]]) */
{
	_bucketSize[0] = bucketSize[0];
	_bucketSize[1] = bucketSize[1];
	_imageSize[0] = imageSize[0];
	_imageSize[1] = imageSize[1];

	allocate();
}

FrameBuffer::~FrameBuffer()
{
}

void FrameBuffer::allocate(void)
{
	unsigned int xbuckets = _imageSize[0] / _bucketSize[0];
	unsigned int ybuckets = _imageSize[1] / _bucketSize[1];
	for (unsigned int i = 0; i < xbuckets; i++)
	{
		std::vector<Bucket> vb;
		for (unsigned int j = 0; j < ybuckets; j++)
		{
			Bucket b;
			vb.push_back(b);
		}
		_buckets.push_back(vb);
	}
}

unsigned int * FrameBuffer::bucketSize(void) const
{
	return const_cast<unsigned int*>(_bucketSize);
}

unsigned int * FrameBuffer::imageSize(void) const
{
	return const_cast<unsigned int*>(_imageSize);
}

void FrameBuffer::addPrimitive(const ReyesPrimitivePtr & primitive, const AABBox & bound)
{
	// bound should be in raster space
	Vector3 min = bound.min();
	// get the upper left corner of the bound and stick it in the corresponding bucket
	unsigned int i = static_cast<unsigned int>(min[0]) / _bucketSize[0];
	unsigned int j = static_cast<unsigned int>(min[1]) / _bucketSize[1];

	std::cerr << "Framebuffer::addPrimitive - adding primitive to bucket [" << i << ", " << j << "]." << std::endl;

	assert(i < (_imageSize[0] / _bucketSize[0]));
	assert(j < (_imageSize[1] / _bucketSize[1]));

	_buckets[i][j].addPrimitive(primitive);
}

void FrameBuffer::render(void)
{
	std::cerr << "FrameBuffer::render - begin rendering framebuffer." << std::endl;

	/*
		go through the buckets and render their contents
	*/
	std::cerr << "FrameBuffer Size: " << _imageSize[0] << "x" << _imageSize[1] << std::endl;
	std::cerr << "Bucket Size: " << _bucketSize[0] << "x" << _bucketSize[1] << std::endl;
	unsigned int xbuckets = _imageSize[0] / _bucketSize[0];
	unsigned int ybuckets = _imageSize[1] / _bucketSize[1];
	std::cerr << "Buckets Required: " << xbuckets << "x" << ybuckets << std::endl;
	for (unsigned int i = 0; i < xbuckets; i++)
	{
		for (unsigned int j = 0; j < ybuckets; j++)
		{
			std::cerr << "FrameBuffer::render - bucket[" << i << "][" << j << "]" << std::endl;
			_buckets[i][j].render();
		}
	}

	std::cerr << "FrameBuffer::render - finished rendering framebuffer." << std::endl;
}
