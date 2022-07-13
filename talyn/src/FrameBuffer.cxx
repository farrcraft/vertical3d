/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "FrameBuffer.h"

#include <iostream>

#include <boost/make_shared.hpp>

namespace Talyn {

    FrameBuffer::FrameBuffer(unsigned int width, unsigned int height, unsigned int depth) : width_(width), height_(height) {
        for (unsigned int i = 0; i < depth; i++) {
            plane_t plane;
            for (unsigned int row = 0; row < height; row++) {
                std::vector<float> plane_row;
                for (unsigned int col = 0; col < width; col++) {
                    plane_row.push_back(0.0f);
                }
                plane.push_back(plane_row);
            }
            planes_.push_back(plane);
        }
    }

    FrameBuffer::~FrameBuffer() {
    }

    boost::shared_ptr<v3d::image::Image> FrameBuffer::render() {
        auto channels = planes_.size();
        auto bpp = channels * 8;
        boost::shared_ptr<v3d::image::Image> image = boost::make_shared<v3d::image::Image>(width_, height_, bpp);
        unsigned char* data = image->data();
        unsigned int index = 0;
        for (unsigned int row = 0; row < height_; row++) {
            for (unsigned int col = 0; col < width_; col++) {
                index = (row * width_) + col;

                for (unsigned int i = 0; i < channels; i++) {
                    data[index + i] = static_cast<unsigned char>(planes_[i][row][col] * 255.0f);
                }
            }
        }
        return image;
    }

};  // namespace Talyn
