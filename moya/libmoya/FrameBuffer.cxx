/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "FrameBuffer.h"
#include "ReyesPrimitive.h"

#include <cassert>
#include <iostream>

using namespace v3d::moya;

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

void FrameBuffer::allocate()
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

unsigned int * FrameBuffer::bucketSize() const
{
    return const_cast<unsigned int*>(_bucketSize);
}

unsigned int * FrameBuffer::imageSize() const
{
    return const_cast<unsigned int*>(_imageSize);
}

void FrameBuffer::addPrimitive(const boost::shared_ptr<ReyesPrimitive> & primitive, const v3d::type::AABBox & bound)
{
    // bound should be in raster space
    glm::vec3 min = bound.min();
    // get the upper left corner of the bound and stick it in the corresponding bucket
    unsigned int i = static_cast<unsigned int>(min[0]) / _bucketSize[0];
    unsigned int j = static_cast<unsigned int>(min[1]) / _bucketSize[1];

    //log4cxx::LoggerPtr logger(log4cxx::Logger::getLogger("v3d.moya"));
    //LOG4CXX_DEBUG(logger, "Framebuffer::addPrimitive - adding primitive to bucket [" << i << ", " << j << "].");

    assert(i < (_imageSize[0] / _bucketSize[0]));
    assert(j < (_imageSize[1] / _bucketSize[1]));

    _buckets[i][j].addPrimitive(primitive);
}

void FrameBuffer::render()
{
    //log4cxx::LoggerPtr logger(log4cxx::Logger::getLogger("v3d.moya"));
    //LOG4CXX_DEBUG(logger, "FrameBuffer::render - begin rendering framebuffer.");

    /*
        go through the buckets and render their contents
    */
    //LOG4CXX_DEBUG(logger, "FrameBuffer Size: " << _imageSize[0] << "x" << _imageSize[1]);
    //LOG4CXX_DEBUG(logger, "Bucket Size: " << _bucketSize[0] << "x" << _bucketSize[1]);
    unsigned int xbuckets = _imageSize[0] / _bucketSize[0];
    unsigned int ybuckets = _imageSize[1] / _bucketSize[1];
    //LOG4CXX_DEBUG(logger, "Buckets Required: " << xbuckets << "x" << ybuckets);
    for (unsigned int i = 0; i < xbuckets; i++)
    {
        for (unsigned int j = 0; j < ybuckets; j++)
        {
            ///LOG4CXX_DEBUG(logger, "FrameBuffer::render - bucket[" << i << "][" << j << "]");
            _buckets[i][j].render();
        }
    }

    //LOG4CXX_DEBUG(logger, "FrameBuffer::render - finished rendering framebuffer.");
}
