/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "TextureAtlas.h"

#include <api/log/Logger.h>

#include <algorithm>
#include <cassert>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>

#include <boost/shared_ptr.hpp>

#include "Image.h"

namespace {

/**
 * Texels held empty around every region, so that a sampler reading a region's edge texel
 * cannot reach into whatever was packed beside it. One texel is enough for bilinear
 * filtering. A mipmapped atlas would need a gutter that grows with the mip chain.
 **/
constexpr unsigned int gutter = 1;

};  // namespace

namespace v3d::image {
TextureAtlas::TextureAtlas(unsigned int width, unsigned int height, unsigned int depth, const boost::shared_ptr<v3d::log::Logger>& logger) :
    width_(width),
    height_(height),
    depth_(depth),
    used_(0),
    id_(0),
    logger_(logger) {
    if (depth != 1 && depth != 3 && depth != 4) {
        const std::string err = "Invalid texture atlas depth - must be 1, 3, or 4";
        logger_->get()->error(err);
        throw std::runtime_error(err);
    }

    // 1 pixel border
    nodes_.push_back(glm::ivec3(1, 1, width - 2));

    image_.reset(new Image(glm::uvec2(width, height), static_cast<uint8_t>(depth * 8)));
}

TextureAtlas::~TextureAtlas() {
}

unsigned int TextureAtlas::width() const {
    return width_;
}

unsigned int TextureAtlas::height() const {
    return height_;
}

unsigned int TextureAtlas::depth() const {
    return depth_;
}

unsigned int TextureAtlas::id() const {
    return id_;
}

glm::ivec4 TextureAtlas::region(unsigned int width, unsigned int height) {
    const glm::ivec4 cell = allocate(width + (gutter * 2), height + (gutter * 2));
    if (cell.x < 0) {
        return cell;
    }
    return glm::ivec4(cell.x + gutter, cell.y + gutter, width, height);
}

glm::ivec4 TextureAtlas::allocate(unsigned int width, unsigned int height) {
    glm::ivec3 node;
    int bestHeight = INT_MAX;
    int bestWidth = INT_MAX;
    int bestIndex = -1;
    glm::ivec4 region(0, 0, width, height);
    for (unsigned int i = 0; i < nodes_.size(); ++i) {
        const int y = fit(i, glm::uvec2(width, height));
        if (y >= 0) {
            node = nodes_[i];
            if (((y + static_cast<int>(height)) < bestHeight) ||
                (((y + static_cast<int>(height)) == bestHeight) && (node.z < bestWidth))) {
                bestHeight = y + static_cast<int>(height);
                bestIndex = static_cast<int>(i);
                bestWidth = node.z;
                region.x = node.x;
                region.y = y;
            }
        }
    }

    if (bestIndex == -1) {
        region.x = -1;
        region.y = -1;
        region.z = 0;
        region.w = 0;
        return region;
    }

    node.x = region.x;
    node.y = region.y + static_cast<int>(height);
    node.z = static_cast<int>(width);
    nodes_.insert(nodes_.begin() + bestIndex, node);

    glm::ivec3 prev;
    for (unsigned int i = bestIndex + 1; i < nodes_.size(); ++i) {
        prev = nodes_[i - 1];

        if (nodes_[i].x < (prev.x + prev.z)) {
            const int shrink = prev.x + prev.z - nodes_[i].x;
            nodes_[i].x += shrink;
            nodes_[i].z -= shrink;
            if (nodes_[i].z <= 0) {
                nodes_.erase(nodes_.begin() + i);
                --i;
            } else {
                break;
            }
        } else {
            break;
        }
    }
    merge();
    used_ += static_cast<size_t>(width) * height;
    return region;
}

/**
 **/
int TextureAtlas::fit(unsigned int index, const glm::uvec2& size) {
    const unsigned int width = size.x;
    const unsigned int height = size.y;
    glm::ivec3 node = nodes_[index];
    const int x = node.x;
    if ((x + width) > (width_ - 1)) {
        return -1;
    }
    int y = node.y;
    int widthLeft = static_cast<int>(width);
    unsigned int nodeIndex = index;
    while (widthLeft > 0) {
        node = nodes_[nodeIndex];
        y = std::max(y, node.y);
        if ((y + height) > (height_ - 1)) {
            return -1;
        }
        widthLeft -= node.z;
        ++nodeIndex;
    }
    return y;
}

void TextureAtlas::merge() {
    for (unsigned int i = 0; i < nodes_.size() - 1; ++i) {
        if (nodes_[i].y == nodes_[i + 1].y) {
            nodes_[i].z += nodes_[i + 1].z;
            nodes_.erase(nodes_.begin() + (i + 1));
            --i;
        }
    }
}

void TextureAtlas::region(const glm::ivec4& area, unsigned char* data, unsigned int stride) {
    const unsigned int x = static_cast<unsigned int>(area.x);
    const unsigned int y = static_cast<unsigned int>(area.y);
    const unsigned int width = static_cast<unsigned int>(area.z);
    const unsigned int height = static_cast<unsigned int>(area.w);
    assert(x > 0);
    assert(y > 0);
    assert(x < (width_ - 1));
    assert((x + width) <= (width_ - 1));
    assert(y < (height_ - 1));
    assert((y + height) <= (height_ - 1));

    const size_t charsize = sizeof(char);
    unsigned char* imgData = image_->data();
    for (unsigned int i = 0; i < height; ++i) {
        memcpy(imgData + (((static_cast<size_t>(y + i) * width_) + x) * charsize * depth_),
            data + ((static_cast<size_t>(i) * stride) * charsize), width * charsize * depth_);
    }
}

boost::shared_ptr<Image> TextureAtlas::image() {
    return image_;
}

};  // namespace v3d::image
